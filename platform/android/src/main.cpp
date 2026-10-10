#include <android/log.h>
#include <android_native_app_glue.h>

#include <exception>

#include <voxen/core/Engine.hpp>
#include <voxen/render/VulkanContext.hpp>
#include <voxen/render/XrSwapchainRenderer.hpp>
#include <voxen/xr/XrContext.hpp>
#include <voxen/xr/XrSession.hpp>

namespace {
constexpr auto kTag = "Voxen";

void logError(const char* message) {
    __android_log_print(ANDROID_LOG_ERROR, kTag, "%s", message);
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
            xr.passthroughEnabled());
        __android_log_print(ANDROID_LOG_INFO, kTag, "XR session initialized; initializing renderer");
        renderer.initialize(
            xr.instance(),
            xr.systemId(),
            session.handle(),
            vulkan.device(),
            vulkan.graphicsQueue(),
            vulkan.graphicsQueueFamily());

        __android_log_print(ANDROID_LOG_INFO, kTag, "Renderer initialized");
        __android_log_print(
            ANDROID_LOG_INFO,
            kTag,
            "OpenXR session created: instance=%p system=%llu",
            reinterpret_cast<void*>(xr.instance()),
            static_cast<unsigned long long>(xr.systemId()));

        bool running = true;
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
            if (session.running()) {
                session.frame(renderer);
            }
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
