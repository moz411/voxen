#include <android/log.h>
#include <android_native_app_glue.h>
#include <jni.h>

#include <exception>
#include <memory>
#include "AndroidVideoDecoder.hpp"
#include "AndroidVideoImage.hpp"
#include "VideoFrameDescriptor.hpp"
#include "AndroidVideoBuffer.hpp"
#include <filesystem>
#include <chrono>
#include <vector>
#include <voxen/scene/SceneRepository.hpp>

#include <voxen/core/Engine.hpp>
#include <voxen/render/VulkanContext.hpp>
#include <voxen/render/XrSwapchainRenderer.hpp>
#include <voxen/xr/XrContext.hpp>
#include <voxen/xr/XrSession.hpp>

namespace {
constexpr auto kTag = "Voxen";

// Diagnostic visual preview: low-rate RGBA upload via existing panorama pipeline.
// NOT the production zero-copy MediaCodec frame renderer.
bool uploadMp4Preview(android_app* app, voxen::render::XrSwapchainRenderer& renderer,
                      const std::filesystem::path& file, std::int64_t timeUs) {
    if (!app || !app->activity || !app->activity->vm || !app->activity->clazz)
        return false;
    JNIEnv* env=nullptr;
    JavaVM* vm=app->activity->vm;
    const bool attached=vm->GetEnv(reinterpret_cast<void**>(&env),JNI_VERSION_1_6)!=JNI_OK;
    if (attached && vm->AttachCurrentThread(&env,nullptr)!=JNI_OK) return false;
    bool ok=false;
    jclass cls=env->GetObjectClass(app->activity->clazz);
    jmethodID method=cls ? env->GetMethodID(cls,"extractVideoPreviewRgba",
        "(Ljava/lang/String;J)[B") : nullptr;
    jstring name=env->NewStringUTF(file.string().c_str());
    auto bytes=static_cast<jbyteArray>(method && name ?
        env->CallObjectMethod(app->activity->clazz,method,name,static_cast<jlong>(timeUs)) : nullptr);
    if(!env->ExceptionCheck() && bytes && env->GetArrayLength(bytes)==512*256*4) {
        voxen::render::PanoramaImage frame;
        frame.width=512;
        frame.height=256;
        frame.rgba.resize(512*256*4);
        env->GetByteArrayRegion(bytes,0,512*256*4,
            reinterpret_cast<jbyte*>(frame.rgba.data()));
        if(!env->ExceptionCheck()) ok=renderer.submitVideoPreview(std::move(frame));
    }
    if(env->ExceptionCheck()) {env->ExceptionDescribe();env->ExceptionClear();}
    if(bytes)env->DeleteLocalRef(bytes);
    if(name)env->DeleteLocalRef(name);
    if(cls)env->DeleteLocalRef(cls);
    if(attached)vm->DetachCurrentThread();
    return ok;
}

void logError(const char* message) {
    __android_log_print(ANDROID_LOG_ERROR, kTag, "%s", message);
}
// Android's permission prompt is asynchronous. Never assume that a
// manifest declaration implies the user granted Meta spatial data access.
// Poll the Android SAF import queue without holding references to Activity JNI objects.
void openPanoramaPicker(android_app* app) {
    if (!app || !app->activity || !app->activity->vm || !app->activity->clazz) return;
    JNIEnv* env=nullptr;
    JavaVM* vm=app->activity->vm;
    const bool attached=vm->GetEnv(reinterpret_cast<void**>(&env),JNI_VERSION_1_6)!=JNI_OK;
    if(attached && vm->AttachCurrentThread(&env,nullptr)!=JNI_OK)return;
    jclass cls=env->GetObjectClass(app->activity->clazz);
    if(cls) {
        jmethodID method=env->GetMethodID(cls,"openPanoramaPicker","()V");
        if(method && !env->ExceptionCheck())
            env->CallVoidMethod(app->activity->clazz,method);
        env->DeleteLocalRef(cls);
    }
    if(env->ExceptionCheck()) {env->ExceptionDescribe();env->ExceptionClear();}
    if(attached)vm->DetachCurrentThread();
}
bool pollPanoramaImports(android_app* app) {
    if (!app || !app->activity || !app->activity->vm || !app->activity->clazz) return false;
    bool imported=false;
    JNIEnv* env = nullptr;
    JavaVM* vm = app->activity->vm;
    const bool attached = vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK;
    if (attached && vm->AttachCurrentThread(&env, nullptr) != JNI_OK) return false;
    jclass activityClass = env->GetObjectClass(app->activity->clazz);
    if (activityClass) {
        jmethodID poll = env->GetMethodID(activityClass, "pollImportedPanorama",
                                          "()Ljava/lang/String;");
        if (poll && !env->ExceptionCheck()) {
            for (unsigned n = 0; n < 8; ++n) {
                auto value = static_cast<jstring>(
                    env->CallObjectMethod(app->activity->clazz, poll));
                if (env->ExceptionCheck() || !value) break;
                const char* utf = env->GetStringUTFChars(value, nullptr);
                if (utf) {
                    __android_log_print(ANDROID_LOG_INFO, "VoxenPanorama",
                        "Panorama import ready: %s", utf);
                    imported=true;
                    // TODO: enqueue the private path for validated decode and Vulkan upload.
                    env->ReleaseStringUTFChars(value, utf);
                }
                env->DeleteLocalRef(value);
            }
        }
        env->DeleteLocalRef(activityClass);
    }
    if (env->ExceptionCheck()) {
        env->ExceptionDescribe();
        env->ExceptionClear();
    }
    if (attached) vm->DetachCurrentThread();
    return imported;
}
constexpr const char* kScenePermission="com.oculus.permission.USE_SCENE";

bool scenePermissionGranted(android_app* app, bool requestIfMissing) {
    if(!app || !app->activity || !app->activity->vm || !app->activity->clazz)
        return false;
    JNIEnv* env=nullptr;
    JavaVM* vm=app->activity->vm;
    const bool attached=vm->GetEnv(reinterpret_cast<void**>(&env),JNI_VERSION_1_6)!=JNI_OK;
    if(attached && vm->AttachCurrentThread(&env,nullptr)!=JNI_OK)return false;
    bool granted=false;
    jclass activityClass=env->GetObjectClass(app->activity->clazz);
    if(activityClass) {
        jmethodID check=env->GetMethodID(activityClass,"checkSelfPermission",
                                        "(Ljava/lang/String;)I");
        jstring name=env->NewStringUTF(kScenePermission);
        if(check && name) {
            const jint result=env->CallIntMethod(app->activity->clazz,check,name);
            if(!env->ExceptionCheck())granted=(result==0);
        }
        if(!granted && requestIfMissing && !env->ExceptionCheck()) {
            jmethodID request=env->GetMethodID(activityClass,"requestPermissions",
                                               "([Ljava/lang/String;I)V");
            jclass stringClass=env->FindClass("java/lang/String");
            if(request && stringClass && name) {
                jobjectArray permissions=env->NewObjectArray(1,stringClass,name);
                if(permissions) {
                    env->CallVoidMethod(app->activity->clazz,request,permissions,411);
                    env->DeleteLocalRef(permissions);
                }
            }
            if(stringClass)env->DeleteLocalRef(stringClass);
        }
        if(name)env->DeleteLocalRef(name);
        env->DeleteLocalRef(activityClass);
    }
    if(env->ExceptionCheck()) {
        env->ExceptionDescribe();
        env->ExceptionClear();
    }
    if(attached)vm->DetachCurrentThread();
    return granted;
}
} // namespace

void android_main(android_app* app) {
    voxen::core::Engine engine;
    voxen::xr::XrContext xr;
    voxen::render::VulkanContext vulkan;
    voxen::xr::XrSession session;
    voxen::render::XrSwapchainRenderer renderer;

    __android_log_print(ANDROID_LOG_INFO, kTag, "android_main entered");
    try {
        __android_log_print(ANDROID_LOG_INFO, kTag, "Initializing engine");
        engine.initialize();
        // Metadata is stored in Android app-private storage, not a shared Music folder.
        // No gallery portals are rendered until a persistent Meta room anchor localizes.
        if (app->activity->internalDataPath != nullptr) {
            voxen::gallery::SceneRepository scenes(
                std::filesystem::path(app->activity->internalDataPath) / "voxen-gallery");
            renderer.configureSceneStorage(
                std::filesystem::path(app->activity->internalDataPath) / "voxen-gallery");
            const auto saved = renderer.savedSceneIds();
            __android_log_print(ANDROID_LOG_INFO, kTag,
                "Gallery repository initialized: %zu saved scenes", saved.size());
        } else {
            __android_log_print(ANDROID_LOG_WARN, kTag,
                "Gallery repository unavailable: no private data path");
        }
        bool sceneGranted=scenePermissionGranted(app,true);
        __android_log_print(ANDROID_LOG_INFO,"VoxenScene",
            "Meta scene permission at startup: %s",sceneGranted?"granted":"pending or denied");
        __android_log_print(ANDROID_LOG_INFO, kTag, "Initializing OpenXR");
        xr.initialize(app->activity->vm, app->activity->clazz);
        __android_log_print(ANDROID_LOG_INFO, kTag, "OpenXR initialized; passthrough=%d", xr.passthroughEnabled() ? 1 : 0);
        __android_log_print(ANDROID_LOG_INFO, kTag, "Initializing Vulkan");
        vulkan.initialize(xr.instance(), xr.systemId());
        __android_log_print(ANDROID_LOG_INFO, kTag, "Vulkan initialized");
        __android_log_print(ANDROID_LOG_INFO, kTag, "Initializing XR session");
        session.initialize(
            xr.instance(),
            xr.systemId(),
            vulkan.instance(),
            vulkan.physicalDevice(),
            vulkan.device(),
            vulkan.graphicsQueueFamily(),
            xr.passthroughEnabled(),
            xr.sceneEnabled(),
            sceneGranted);
        __android_log_print(ANDROID_LOG_INFO, kTag, "XR session initialized; initializing renderer");
        renderer.configureMusicJavaVm(app->activity->vm);
        renderer.initialize(
            xr.instance(),
            xr.systemId(),
            vulkan.instance(),
            session.handle(),
            vulkan.device(),
            vulkan.physicalDevice(),
            vulkan.graphicsQueue(),
            vulkan.graphicsQueueFamily());

        __android_log_print(ANDROID_LOG_INFO, kTag, "Renderer initialized");

        // Selection-driven decoder lifecycle; the Vulkan sampler path remains
        // separate until external ownership/fence synchronization is verified.
        std::unique_ptr<voxen::video::AndroidVideoDecoder> videoDecoder;
        std::filesystem::path activeVideoPath;
        unsigned importedVideoFrames = 0;
        auto videoPreviewStart=std::chrono::steady_clock::now();
        auto videoPreviewLast=std::chrono::steady_clock::time_point{};
        __android_log_print(
            ANDROID_LOG_INFO,
            kTag,
            "OpenXR session created: instance=%p system=%llu",
            reinterpret_cast<void*>(xr.instance()),
            static_cast<unsigned long long>(xr.systemId()));

        bool running = true;
        unsigned permissionPollCounter=0;
        while (running && app->destroyRequested == 0) {
            int events = 0;
            android_poll_source* source = nullptr;

            // xrWaitFrame paces active XR frames. Before READY, wake periodically so
            // OpenXR events cannot be starved by the Android looper.
            const int timeoutMs = session.running() ? 0 : 10;
            const int pollResult = ALooper_pollOnce(
                timeoutMs,
                nullptr,
                &events,
                reinterpret_cast<void**>(&source));

            if (pollResult >= 0 && source != nullptr) {
                source->process(app, source);
            }

            if (app->destroyRequested != 0) {
                break;
            }

            running = session.pollEvents();
            // Renderer UI determines the active MP4. A selection change
            // releases the previous decoder and opens the chosen clip.
            const auto requestedVideo = renderer.selectedBackgroundVideo();
            if (requestedVideo != activeVideoPath) {
                if (videoDecoder) {
                    videoDecoder->close();
                    videoDecoder.reset();
                }
                activeVideoPath = requestedVideo;
                importedVideoFrames = 0;
                videoPreviewStart=std::chrono::steady_clock::now();
                videoPreviewLast={};
                if (!requestedVideo.empty() && vulkan.videoBufferImportEnabled() &&
                    std::filesystem::is_regular_file(requestedVideo)) {
                    videoDecoder = std::make_unique<voxen::video::AndroidVideoDecoder>(
                        app->activity->vm, app->activity->clazz);
                    if (!videoDecoder->valid() ||
                        !videoDecoder->open(requestedVideo.string(), true)) {
                        videoDecoder.reset();
                        __android_log_print(ANDROID_LOG_ERROR, "VoxenVideo",
                            "Could not open selected MP4: %s", requestedVideo.c_str());
                    } else {
                        __android_log_print(ANDROID_LOG_INFO, "VoxenVideo",
                            "Selected MP4 decoder started: %s", requestedVideo.c_str());
                    }
                }
            }

            // Frame ownership extends across both eye submissions. The current
            // renderer waits for its GPU fence before session.frame returns.
            voxen::video::AndroidVideoDecoder::Frame videoFrame{};
            voxen::video::AndroidVideoImage videoImage{};
            if (videoDecoder) {
                videoFrame = videoDecoder->acquireLatestFrame();
                if (videoFrame.buffer && importedVideoFrames == 0) {
                    const auto format = voxen::video::inspectVideoBuffer(
                        vulkan.device(), videoFrame.buffer);
                    if (format.supported) {
                        const bool ok = videoImage.importBuffer(
                            vulkan.physicalDevice(), vulkan.device(), videoFrame.buffer);
                        if (ok) {
                            voxen::video::VideoFrameDescriptor descriptor;
                            const bool bound = descriptor.create(vulkan.device(), videoImage,
                                videoImage.combinedSamplerDescriptorCount());
                            __android_log_print(bound ? ANDROID_LOG_INFO : ANDROID_LOG_ERROR,
                                "VoxenVideo", "YCbCr immutable descriptor: %s",
                                bound ? "OK" : "FAILED");
                            // Descriptor must be destroyed before its immutable sampler.
                            descriptor.reset();
                        }
                        __android_log_print(ok ? ANDROID_LOG_INFO : ANDROID_LOG_ERROR,
                            "VoxenVideo", "Selected MP4 first frame import: %s",
                            ok ? "OK" : "FAILED");
                    } else {
                        __android_log_print(ANDROID_LOG_WARN, "VoxenVideo",
                            "Selected MP4 buffer not sampleable");
                    }
                    ++importedVideoFrames;
                }
            }
            // 2 fps, 512x256, deliberately low-rate diagnostic visual path.
            // The regular YCbCr importer remains a separate experiment.
            if(!requestedVideo.empty() && std::filesystem::is_regular_file(requestedVideo)) {
                const auto now=std::chrono::steady_clock::now();
                if(videoPreviewLast.time_since_epoch().count()==0 ||
                   now-videoPreviewLast>=std::chrono::milliseconds(500)) {
                    videoPreviewLast=now;
                    const auto elapsed=std::chrono::duration_cast<std::chrono::microseconds>(
                        now-videoPreviewStart).count();
                    if(uploadMp4Preview(app,renderer,requestedVideo,elapsed)) {
                        if(elapsed<600000)
                            __android_log_print(ANDROID_LOG_INFO,"VoxenVideo",
                                "MP4 panorama RGBA preview visible path active");
                    }
                }
            }
            if(pollPanoramaImports(app))renderer.rescanPanoramaAssets();
            if(renderer.consumePanoramaImportRequest())openPanoramaPicker(app);
            // Poll the permission state after Android handles the system dialog.
            // Re-request spatial discovery once the user grants access.
            if(!sceneGranted && (++permissionPollCounter % 120u)==0u &&
               scenePermissionGranted(app,false)) {
                sceneGranted=true;
                __android_log_print(ANDROID_LOG_INFO,"VoxenScene",
                    "Meta scene permission granted; refreshing scene discovery");
                session.setScenePermissionGranted(true);
            }
            if (session.running()) {
                session.frame(renderer);
            }
            // Release only after both eyes have completed their GPU submissions.
            // This also covers the no-session case (no GPU work was submitted).
            videoImage.reset();
            if(videoDecoder && videoFrame.buffer)
                videoDecoder->releaseFrame(videoFrame);
        }
    } catch (const std::exception& error) {
        logError(error.what());
        ANativeActivity_finish(app->activity);
    } catch (...) {
        logError("Unknown fatal error");
        ANativeActivity_finish(app->activity);
    }

    renderer.shutdown();
    session.shutdown();
    vulkan.shutdown();
    xr.shutdown();
    engine.shutdown();
}
