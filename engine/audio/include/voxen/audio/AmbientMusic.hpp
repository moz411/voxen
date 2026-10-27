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
    // Lock-free, allocation-free triggers consumed on the AAudio callback.
    // type: 0 launch, 1 hit, 2 final explosion
    void triggerEffect(int type) noexcept {
        if(type==0)shots_.fetch_add(1,std::memory_order_relaxed);
        else if(type==1)hits_.fetch_add(1,std::memory_order_relaxed);
        else if(type==2)explosions_.fetch_add(1,std::memory_order_relaxed);
    }
private:
#ifdef __ANDROID__
    static aaudio_data_callback_result_t callback(AAudioStream*,void*,void*,int32_t);
    AAudioStream* stream_=nullptr;
#endif
    std::atomic<bool> ready_{false},playing_{false};
    std::atomic<int> track_{0};
    std::atomic<float> volume_{0.45f};
    std::atomic<uint32_t> shots_{0}, hits_{0}, explosions_{0};
    struct EffectVoice { int type=-1; int remaining=0; int total=0; float phase=0.f; };
    EffectVoice effects_[12]{};
    double phase_=0.;
    float noise_=0.,filter_=0.,wave_=0.;
    uint32_t seed_=0x19781978u;
    int sampleRate_=48000;
};
} // namespace voxen::audio
