#include <android/log.h>
#include <android_native_app_glue.h>

#include <exception>

#include <voxen/core/Engine.hpp>
#include <voxen/render/VulkanContext.hpp>
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

    try {
        engine.initialize();
        xr.initialize(app->activity->vm, app->activity->clazz);
        vulkan.initialize(xr.instance(), xr.systemId());
        session.initialize(
            xr.instance(),
            xr.systemId(),
            vulkan.instance(),
            vulkan.physicalDevice(),
            vulkan.device(),
            vulkan.graphicsQueueFamily());

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

            // Never block while an XR session is running: xrWaitFrame is then
            // the frame pacer. Before READY, block until Android has work.
            const int timeoutMs = session.running() ? 0 : -1;
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
                session.frame();
            }
        }
    } catch (const std::exception& error) {
        logError(error.what());
        ANativeActivity_finish(app->activity);
    } catch (...) {
        logError("Unknown fatal error");
        ANativeActivity_finish(app->activity);
    }

    session.shutdown();
    vulkan.shutdown();
    xr.shutdown();
    engine.shutdown();
}
