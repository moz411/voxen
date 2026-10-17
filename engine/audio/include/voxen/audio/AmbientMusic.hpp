#pragma once
#include <atomic>
#include <cstdint>
#ifdef __ANDROID__
#include <aaudio/AAudio.h>
#endif

namespace voxen::audio {
// Lightweight, original procedural ambient loops: no external copyrighted audio.
// The render callback is realtime-safe (no allocations, locks or logging).
class AmbientMusic {
public:
    bool start() noexcept;
    void stop() noexcept;
    void configure(int track, bool playing, float volume) noexcept {
        track_.store(track);playing_.store(playing);volume_.store(volume);
    }
    [[nodiscard]] bool ready() const noexcept {return ready_.load();}
private:
#ifdef __ANDROID__
    static aaudio_data_callback_result_t callback(AAudioStream*,void*,void*,int32_t);
    AAudioStream* stream_=nullptr;
#endif
    std::atomic<bool> ready_{false},playing_{false};
    std::atomic<int> track_{0};
    std::atomic<float> volume_{0.45f};
    double phase_=0.;
    float noise_=0.,filter_=0.,wave_=0.;
    uint32_t seed_=0x19781978u;
    int sampleRate_=48000;
};
} // namespace voxen::audio
