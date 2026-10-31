#include "AndroidVideoDecoder.hpp"
#ifdef __ANDROID__
#include <android/log.h>
#include <android/hardware_buffer_jni.h>
#include <algorithm>

namespace voxen::video {
namespace {
constexpr const char* kTag = "VoxenVideo";
void clearException(JNIEnv* env, const char* context) {
    if (!env->ExceptionCheck()) return;
    __android_log_print(ANDROID_LOG_ERROR, kTag, "JNI exception: %s", context);
    env->ExceptionDescribe();
    env->ExceptionClear();
}
}

JNIEnv* AndroidVideoDecoder::attach(bool& shouldDetach) const {
    shouldDetach = false;
    if (!vm_) return nullptr;
    JNIEnv* env = nullptr;
    if (vm_->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) == JNI_OK)
        return env;
    if (vm_->AttachCurrentThread(&env, nullptr) != JNI_OK) return nullptr;
    shouldDetach = true;
    return env;
}

void AndroidVideoDecoder::detach(bool shouldDetach) const {
    if (shouldDetach) vm_->DetachCurrentThread();
}

AndroidVideoDecoder::AndroidVideoDecoder(JavaVM* vm, jobject activity) : vm_(vm) {
    if (!vm_ || !activity) return;
    bool needsDetach = false;
    JNIEnv* env = attach(needsDetach);
    if (!env) return;
    // Use Activity's class loader (FindClass on a native app thread cannot
    // reliably resolve application classes).
    jclass activityClass = env->GetObjectClass(activity);
    jclass classClass = env->FindClass("java/lang/Class");
    jmethodID loaderMethod = activityClass
        ? env->GetMethodID(activityClass, "getClassLoader", "()Ljava/lang/ClassLoader;") : nullptr;
    jobject loader = loaderMethod ? env->CallObjectMethod(activity, loaderMethod) : nullptr;
    jclass loaderClass = loader ? env->GetObjectClass(loader) : nullptr;
    jmethodID loadClass = loaderClass ? env->GetMethodID(
        loaderClass, "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;") : nullptr;
    jstring name = env->NewStringUTF("org.voxen.xr.media.VideoDecoder");
    jobject clsObject = (loader && loadClass && name)
        ? env->CallObjectMethod(loader, loadClass, name) : nullptr;
    clearException(env, "load VideoDecoder");
    jclass cls = static_cast<jclass>(clsObject);
    jmethodID ctor = cls ? env->GetMethodID(cls, "<init>", "()V") : nullptr;
    jobject local = ctor ? env->NewObject(cls, ctor) : nullptr;
    clearException(env, "construct VideoDecoder");
    if (local) decoder_ = env->NewGlobalRef(local);
    if (local) env->DeleteLocalRef(local);
    if (cls) env->DeleteLocalRef(cls);
    if (name) env->DeleteLocalRef(name);
    if (loaderClass) env->DeleteLocalRef(loaderClass);
    if (loader) env->DeleteLocalRef(loader);
    if (classClass) env->DeleteLocalRef(classClass);
    if (activityClass) env->DeleteLocalRef(activityClass);
    detach(needsDetach);
}

AndroidVideoDecoder::~AndroidVideoDecoder() { close(); }

void AndroidVideoDecoder::invokeVoid(const char* method, const char* signature) {
    if (!decoder_) return;
    bool needsDetach = false;
    JNIEnv* env = attach(needsDetach);
    if (!env) return;
    jclass cls = env->GetObjectClass(decoder_);
    jmethodID id = cls ? env->GetMethodID(cls, method, signature) : nullptr;
    if (id) env->CallVoidMethod(decoder_, id);
    clearException(env, method);
    if (cls) env->DeleteLocalRef(cls);
    detach(needsDetach);
}

bool AndroidVideoDecoder::open(const std::string& localPath, bool loop) {
    if (!decoder_) return false;
    bool needsDetach = false;
    JNIEnv* env = attach(needsDetach);
    if (!env) return false;
    jclass cls = env->GetObjectClass(decoder_);
    jmethodID id = cls ? env->GetMethodID(cls, "open", "(Ljava/lang/String;Z)V") : nullptr;
    jstring path = env->NewStringUTF(localPath.c_str());
    if (id && path) env->CallVoidMethod(decoder_, id, path, static_cast<jboolean>(loop));
    const bool ok = id && path && !env->ExceptionCheck();
    clearException(env, "open");
    if (path) env->DeleteLocalRef(path);
    if (cls) env->DeleteLocalRef(cls);
    detach(needsDetach);
    return ok; // Async start accepted, not proof of successful decoding.
}

void AndroidVideoDecoder::play() { invokeVoid("play", "()V"); }
void AndroidVideoDecoder::pause() { invokeVoid("pause", "()V"); }

void AndroidVideoDecoder::seekToMs(long long milliseconds) {
    if (!decoder_) return;
    bool needsDetach = false;
    JNIEnv* env = attach(needsDetach);
    if (!env) return;
    jclass cls = env->GetObjectClass(decoder_);
    jmethodID id = cls ? env->GetMethodID(cls, "seekToMs", "(J)V") : nullptr;
    if (id) env->CallVoidMethod(decoder_, id, static_cast<jlong>(std::max(0LL, milliseconds)));
    clearException(env, "seekToMs");
    if (cls) env->DeleteLocalRef(cls);
    detach(needsDetach);
}

AndroidVideoDecoder::Frame AndroidVideoDecoder::acquireLatestFrame() {
    Frame frame;
    if (!decoder_) return frame;
    bool needsDetach = false;
    JNIEnv* env = attach(needsDetach);
    if (!env) return frame;
    jclass cls = env->GetObjectClass(decoder_);
    jmethodID acquire = cls ? env->GetMethodID(
        cls, "acquireLatestFrame", "()Landroid/media/Image;") : nullptr;
    jobject image = acquire ? env->CallObjectMethod(decoder_, acquire) : nullptr;
    if (!env->ExceptionCheck() && image) {
        jclass imageCls = env->GetObjectClass(image);
        jmethodID hardwareBuffer = imageCls ? env->GetMethodID(
            imageCls, "getHardwareBuffer", "()Landroid/hardware/HardwareBuffer;") : nullptr;
        jobject javaBuffer = hardwareBuffer ? env->CallObjectMethod(image, hardwareBuffer) : nullptr;
        if (!env->ExceptionCheck() && javaBuffer) {
            auto* buffer = AHardwareBuffer_fromHardwareBuffer(env, javaBuffer);
            if (buffer) {
                AHardwareBuffer_acquire(buffer);
                frame.buffer = buffer;
                frame.image = env->NewGlobalRef(image);
                if (!frame.image) {
                    AHardwareBuffer_release(frame.buffer);
                    frame.buffer = nullptr;
                }
            }
        }
        if (!frame.image) {
            jmethodID close = imageCls ? env->GetMethodID(imageCls, "close", "()V") : nullptr;
            if (close) env->CallVoidMethod(image, close);
        }
        if (javaBuffer) env->DeleteLocalRef(javaBuffer);
        if (imageCls) env->DeleteLocalRef(imageCls);
        env->DeleteLocalRef(image);
    }
    clearException(env, "acquireLatestFrame");
    if (cls) env->DeleteLocalRef(cls);
    detach(needsDetach);
    return frame;
}

void AndroidVideoDecoder::releaseFrame(Frame& frame) {
    if (frame.image) {
        bool needsDetach = false;
        JNIEnv* env = attach(needsDetach);
        if (env) {
            jclass cls = env->GetObjectClass(frame.image);
            jmethodID id = cls ? env->GetMethodID(cls, "close", "()V") : nullptr;
            if (id) env->CallVoidMethod(frame.image, id);
            clearException(env, "releaseFrame");
            if (cls) env->DeleteLocalRef(cls);
            env->DeleteGlobalRef(frame.image);
            detach(needsDetach);
        }
        frame.image = nullptr;
    }
    if (frame.buffer) AHardwareBuffer_release(frame.buffer);
    frame.buffer = nullptr;
}

void AndroidVideoDecoder::close() {
    if (!decoder_) return;
    invokeVoid("close", "()V");
    bool needsDetach = false;
    JNIEnv* env = attach(needsDetach);
    if (env) {
        env->DeleteGlobalRef(decoder_);
        detach(needsDetach);
    }
    decoder_ = nullptr;
}
}
#endif
