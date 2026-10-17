#include <voxen/audio/Mp3Library.hpp>
#include <algorithm>
#include <cctype>
#include <filesystem>
#ifdef __ANDROID__
#include <android/log.h>
#endif

namespace voxen::audio {
namespace {
constexpr const char* kDir="/sdcard/Android/data/org.voxen.xr/files/Music";
bool isMp3(const std::filesystem::path& path){
    auto s=path.extension().string();
    std::transform(s.begin(),s.end(),s.begin(),[](unsigned char c){return static_cast<char>(std::tolower(c));});
    return s==".mp3";
}
#ifdef __ANDROID__
JNIEnv* envFor(JavaVM* vm){
    if(!vm)return nullptr;
    JNIEnv* env=nullptr;
    if(vm->GetEnv(reinterpret_cast<void**>(&env),JNI_VERSION_1_6)!=JNI_OK)return nullptr;
    return env; // Caller is the attached android_main thread.
}
bool check(JNIEnv* env){
    if(!env->ExceptionCheck())return true;
    env->ExceptionDescribe();
    env->ExceptionClear();
    return false;
}
void invokeVoid(JNIEnv* env,jobject obj,jclass cls,const char* name,const char* sig){
    jmethodID id=env->GetMethodID(cls,name,sig);
    if(id)env->CallVoidMethod(obj,id);
    check(env);
}
#endif
}
#ifdef __ANDROID__
void Mp3Library::initialize(JavaVM* vm){vm_=vm;refresh();}
#endif
void Mp3Library::refresh(){
    paths_.clear();titles_.clear();
    error_.clear();
    try {
        const std::filesystem::path dir(kDir);
        if(!std::filesystem::is_directory(dir)){
            std::filesystem::create_directories(dir);
        }
        for(const auto& item:std::filesystem::directory_iterator(dir)){
            if(!item.is_regular_file()||!isMp3(item.path()))continue;
            paths_.push_back(item.path().string());
        }
        std::sort(paths_.begin(),paths_.end());
        for(const auto& path:paths_)titles_.push_back(std::filesystem::path(path).filename().string());
    }catch(const std::exception& e){error_=e.what();}
    // If active file disappeared, the next update will release its player.
    selected_=-1;
}
void Mp3Library::shutdown() noexcept {
#ifdef __ANDROID__
    JNIEnv* env=envFor(vm_);
    if(env&&player_){
        jclass cls=env->GetObjectClass(player_);
        if(cls){
            invokeVoid(env,player_,cls,"release","()V");
            env->DeleteLocalRef(cls);
        }
        env->DeleteGlobalRef(player_);
    }
    player_=nullptr;
#endif
    selected_=-1;
    wasPlaying_=false;
    lastVolume_=-1.f;
}
void Mp3Library::update(int selection,bool playing,float volume){
#ifdef __ANDROID__
    JNIEnv* env=envFor(vm_);
    if(!env)return;
    volume=std::clamp(volume,0.f,1.f);
    if(selection<0||selection>=static_cast<int>(paths_.size())){
        if(selected_!=-1)shutdown();
        return;
    }
    if(selected_!=selection){
        shutdown();
        error_.clear();
        jclass cls=env->FindClass("android/media/MediaPlayer");
        if(!cls||!check(env)){error_="MediaPlayer class unavailable";return;}
        auto ctor=env->GetMethodID(cls,"<init>","()V");
        jobject local=ctor?env->NewObject(cls,ctor):nullptr;
        if(!local||!check(env)){error_="Cannot initialize MP3 player";env->DeleteLocalRef(cls);return;}
        player_=env->NewGlobalRef(local);
        env->DeleteLocalRef(local);
        auto setDataSource=env->GetMethodID(cls,"setDataSource","(Ljava/lang/String;)V");
        jstring path=env->NewStringUTF(paths_[selection].c_str());
        if(setDataSource)env->CallVoidMethod(player_,setDataSource,path);
        env->DeleteLocalRef(path);
        bool good=check(env);
        auto prepare=env->GetMethodID(cls,"prepare","()V");
        if(good&&prepare)env->CallVoidMethod(player_,prepare);
        good=good&&check(env);
        auto looping=env->GetMethodID(cls,"setLooping","(Z)V");
        if(good&&looping)env->CallVoidMethod(player_,looping,JNI_TRUE);
        good=good&&check(env);
        env->DeleteLocalRef(cls);
        if(!good){error_="MP3 could not be opened or decoded";shutdown();return;}
        selected_=selection;
    }
    if(!player_)return;
    jclass cls=env->GetObjectClass(player_);
    if(!cls)return;
    if(volume!=lastVolume_){
        auto setVolume=env->GetMethodID(cls,"setVolume","(FF)V");
        if(setVolume)env->CallVoidMethod(player_,setVolume,volume,volume);
        check(env);lastVolume_=volume;
    }
    if(playing!=wasPlaying_){
        invokeVoid(env,player_,cls,playing?"start":"pause","()V");
        wasPlaying_=playing;
    }
    env->DeleteLocalRef(cls);
#else
    (void)selection;(void)playing;(void)volume;
#endif
}
}
