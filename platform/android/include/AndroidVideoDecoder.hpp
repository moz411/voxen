#pragma once
#ifdef __ANDROID__
#include <android/hardware_buffer.h>
#include <jni.h>
#include <string>

namespace voxen::video {

// Owns the Java decoder and the JavaVM attachment boundary. Not thread-safe:
// keep control calls on the Android app thread.
class AndroidVideoDecoder final {
public:
    AndroidVideoDecoder(JavaVM* vm, jobject activity);
    ~AndroidVideoDecoder();
    AndroidVideoDecoder(const AndroidVideoDecoder&) = delete;
    AndroidVideoDecoder& operator=(const AndroidVideoDecoder&) = delete;

    bool valid() const noexcept { return decoder_ != nullptr; }
    bool open(const std::string& localPath, bool loop);
    void play();
    void pause();
    void seekToMs(long long milliseconds);
    void close();

    // Image and HardwareBuffer have separate reference counts. The image stays
    // alive until releaseFrame; Vulkan must have completed reading before then.
    struct Frame {
        jobject image = nullptr; // JNI global reference
        AHardwareBuffer* buffer = nullptr; // acquired native reference
    };
    Frame acquireLatestFrame();
    void releaseFrame(Frame& frame);

private:
    JNIEnv* attach(bool& detach) const;
    void detach(bool detach) const;
    void invokeVoid(const char* method, const char* signature);
    JavaVM* vm_ = nullptr;
    jobject decoder_ = nullptr; // global reference
};
}
#endif
