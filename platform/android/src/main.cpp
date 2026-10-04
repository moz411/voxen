#include <android/log.h>
#include <android_native_app_glue.h>

#include <voxen/core/Engine.hpp>

namespace {
constexpr auto kTag = "Voxen";
}

void android_main(android_app* app) {
    app_dummy();

    voxen::core::Engine engine;
    engine.initialize();

    __android_log_print(ANDROID_LOG_INFO, kTag, "Voxen native runtime started");

    bool running = true;
    while (running) {
        int events = 0;
        android_poll_source* source = nullptr;

        const int timeoutMs = app->destroyRequested ? 0 : -1;
        while (ALooper_pollOnce(timeoutMs, nullptr, &events,
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

    engine.shutdown();
}
