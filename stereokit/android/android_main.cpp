#include <android_native_app_glue.h>
#include <pthread.h>
#include <stereokit.h>

extern sk::sk_settings_t app_settings;
extern int voxen_stereokit_main();

static pthread_t worker{};
static bool started = false;
static bool finished = false;

static void* run_app(void*) {
    voxen_stereokit_main();
    finished = true;
    return nullptr;
}
static void on_cmd(android_app* app, int32_t cmd) {
    if (cmd == APP_CMD_INIT_WINDOW && app->window && !started) {
        started = true;
        app_settings.android_activity = app->activity->clazz;
        app_settings.android_java_vm = app->activity->vm;
        pthread_create(&worker, nullptr, run_app, nullptr);
    } else if (cmd == APP_CMD_DESTROY && sk::sk_is_stepping()) {
        sk::sk_quit(sk::quit_reason_user);
    }
}
extern "C" void android_main(android_app* app) {
    app->onAppCmd = on_cmd;
    while (!app->destroyRequested) {
        int events = 0;
        android_poll_source* source = nullptr;
        if (ALooper_pollOnce(started ? 10 : -1, nullptr, &events,
                             reinterpret_cast<void**>(&source)) >= 0 && source)
            source->process(app, source);
        if (finished) {
            ANativeActivity_finish(app->activity);
            break;
        }
    }
    if (sk::sk_is_stepping()) sk::sk_quit(sk::quit_reason_user);
    if (started) pthread_join(worker, nullptr);
}
