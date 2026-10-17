#pragma once
#include <string>
#include <vector>
#ifdef __ANDROID__
#include <jni.h>
#endif
namespace voxen::audio {
class Mp3Library {
public:
#ifdef __ANDROID__
    void initialize(JavaVM* vm);
#endif
    void shutdown() noexcept;
    void refresh();
    void update(int selection,bool playing,float volume);
    [[nodiscard]] const std::vector<std::string>& titles() const noexcept{return titles_;}
    [[nodiscard]] const std::string& error() const noexcept{return error_;}
private:
#ifdef __ANDROID__
    JavaVM* vm_=nullptr;
    jobject player_=nullptr; // GlobalRef to android.media.MediaPlayer
#endif
    std::vector<std::string> paths_,titles_;
    std::string error_;
    int selected_=-1;
    bool wasPlaying_=false;
    float lastVolume_=-1.f;
};
}
