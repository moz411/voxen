#include <android_native_app_glue.h>
#include <pthread.h>
#include <jni.h>
#include <android/log.h>
#include <stereokit.h>

extern sk::sk_settings_t app_settings;
extern int voxen_stereokit_main();

// Android runtime spatial-data permission: manifest declaration alone is insufficient.
// This request triggers the Quest permission dialog on first launch.
static void request_scene_permission(ANativeActivity* activity) {
    JNIEnv* env=nullptr;
    bool attached=false;
    if(activity->vm->GetEnv(reinterpret_cast<void**>(&env),JNI_VERSION_1_6)!=JNI_OK) {
        if(activity->vm->AttachCurrentThread(&env,nullptr)!=JNI_OK) return;
        attached=true;
    }
    jobject context=activity->clazz;
    jclass activityClass=env->GetObjectClass(context);
    jclass packageManager=env->FindClass("android/content/pm/PackageManager");
    jfieldID grantedId=env->GetStaticFieldID(packageManager,"PERMISSION_GRANTED","I");
    const jint granted=env->GetStaticIntField(packageManager,grantedId);
    jstring permission=env->NewStringUTF("com.oculus.permission.USE_SCENE");
    jmethodID check=env->GetMethodID(activityClass,"checkSelfPermission","(Ljava/lang/String;)I");
    const jint state=env->CallIntMethod(context,check,permission);
    __android_log_print(ANDROID_LOG_INFO,"VoxenScene","USE_SCENE permission state=%d",state);
    if(state!=granted) {
        jclass stringClass=env->FindClass("java/lang/String");
        jobjectArray permissions=env->NewObjectArray(1,stringClass,permission);
        jmethodID request=env->GetMethodID(activityClass,"requestPermissions","([Ljava/lang/String;I)V");
        env->CallVoidMethod(context,request,permissions,741);
        __android_log_print(ANDROID_LOG_INFO,"VoxenScene","Requested USE_SCENE runtime permission");
        env->DeleteLocalRef(permissions);
        env->DeleteLocalRef(stringClass);
    }
    env->DeleteLocalRef(permission);
    env->DeleteLocalRef(packageManager);
    env->DeleteLocalRef(activityClass);
    if(env->ExceptionCheck()) { env->ExceptionDescribe(); env->ExceptionClear(); }
    if(attached) activity->vm->DetachCurrentThread();
}
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
        request_scene_permission(app->activity);
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
