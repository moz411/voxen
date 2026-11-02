#pragma once
#include <algorithm>
#include <cmath>

namespace voxen::animation {

// A deterministic scene clock: use the audio playback position when available.
// A wall-clock delta is only a fallback when no soundtrack is playing.
class SceneClock {
public:
    void reset(double seconds = 0.0) noexcept {
        seconds_ = std::max(0.0, seconds);
    }
    void pause(bool paused) noexcept { paused_ = paused; }
    void setRate(double rate) noexcept {
        rate_ = std::clamp(rate, 0.0, 8.0);
    }
    void setLoop(double duration, bool enabled) noexcept {
        duration_ = std::max(0.0, duration);
        looping_ = enabled && duration_ > 0.0;
    }
    void update(double deltaSeconds) noexcept {
        if (!paused_ && std::isfinite(deltaSeconds) && deltaSeconds > 0.0)
            seek(seconds_ + deltaSeconds * rate_);
    }
    void syncToAudio(double playbackSeconds) noexcept {
        if (!paused_ && std::isfinite(playbackSeconds))
            seek(playbackSeconds);
    }
    void seek(double seconds) noexcept {
        if (!std::isfinite(seconds)) return;
        seconds_ = std::max(0.0, seconds);
        if (looping_) seconds_ = std::fmod(seconds_, duration_);
    }
    [[nodiscard]] double seconds() const noexcept { return seconds_; }
private:
    double seconds_ = 0.0;
    double duration_ = 0.0;
    double rate_ = 1.0;
    bool paused_ = false;
    bool looping_ = false;
};
} // namespace voxen::animation
