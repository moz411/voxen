#pragma once
#include <voxen/animation_timeline.h>
#include <algorithm>
#include <cmath>

namespace voxen::animation {

// Stateful attack/release smoothing; update once per scene frame, not per Voxen.
// Silence and paused playback converge to zero without a stuck beat impulse.
class AudioEnvelope {
public:
    void reset() noexcept { current_ = {}; }
    [[nodiscard]] AudioFeatures update(const AudioFeatures& input,
                                       double deltaSeconds, bool playing) noexcept {
        if (!std::isfinite(deltaSeconds) || deltaSeconds < 0.0) return current_;
        const float dt = static_cast<float>(std::min(deltaSeconds, 0.25));
        auto follow = [dt](float current, float target, float attack, float release) {
            target = std::clamp(std::isfinite(target) ? target : 0.f, 0.f, 1.f);
            const float tau = target > current ? attack : release;
            const float alpha = 1.f - std::exp(-dt / tau);
            return current + alpha * (target - current);
        };
        current_.rms = follow(current_.rms, playing ? input.rms : 0.f, 0.03f, 0.20f);
        current_.bass = follow(current_.bass, playing ? input.bass : 0.f, 0.03f, 0.22f);
        current_.mid = follow(current_.mid, playing ? input.mid : 0.f, 0.025f, 0.18f);
        current_.treble = follow(current_.treble, playing ? input.treble : 0.f, 0.02f, 0.15f);
        current_.onset = follow(current_.onset, playing ? input.onset : 0.f, 0.01f, 0.10f);
        current_.timestampSeconds = input.timestampSeconds;
        return current_;
    }
    [[nodiscard]] const AudioFeatures& current() const noexcept { return current_; }
private:
    AudioFeatures current_{};
};
} // namespace voxen::animation
