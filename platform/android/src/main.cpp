#include <android/log.h>
#include <android_native_app_glue.h>

#include <exception>

#include <voxen/core/Engine.hpp>
#include <voxen/render/VulkanContext.hpp>
#include <voxen/xr/XrContext.hpp>

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

    try {
        engine.initialize();
        xr.initialize(app->activity->vm, app->activity->clazz);
        vulkan.initialize(xr.instance(), xr.systemId());

        __android_log_print(
            ANDROID_LOG_INFO,
            kTag,
            "OpenXR + Vulkan initialized: instance=%p system=%llu",
            reinterpret_cast<void*>(xr.instance()),
            static_cast<unsigned long long>(xr.systemId()));

        bool running = true;
        while (running) {
            int events = 0;
            android_poll_source* source = nullptr;

            const int timeoutMs = app->destroyRequested ? 0 : -1;
            while (ALooper_pollOnce(
                       timeoutMs,
                       nullptr,
                       &events,
                       reinterpret_cast<void**>(&source)) >= 0) {
                if (source != nullptr) {
                    source->process(app, source);
                }
                if (app->destroyRequested != 0) {
                    running = false;
                    break;
                }
            }
        }
    } catch (const std::exception& error) {
        logError(error.what());
        ANativeActivity_finish(app->activity);
    } catch (...) {
        logError("Unknown fatal error");
        ANativeActivity_finish(app->activity);
    }

    vulkan.shutdown();
    xr.shutdown();
    engine.shutdown();
}
