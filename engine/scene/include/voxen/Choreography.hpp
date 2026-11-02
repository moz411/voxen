#pragma once
#include <voxen/animation_timeline.h>
#include <array>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace voxen::animation {

enum class Channel : std::uint8_t {
    PositionX, PositionY, PositionZ,
    RotationX, RotationY, RotationZ,
    Scale, Hue, Brightness, Deformation, Speed,
    Count
};

struct AnimatedValues {
    std::array<float, static_cast<std::size_t>(Channel::Count)> values{};
    AnimatedValues() noexcept {
        values[static_cast<std::size_t>(Channel::Scale)] = 1.f;
        values[static_cast<std::size_t>(Channel::Brightness)] = 1.f;
        values[static_cast<std::size_t>(Channel::Speed)] = 1.f;
    }
    float& operator[](Channel channel) noexcept {
        return values[static_cast<std::size_t>(channel)];
    }
    float operator[](Channel channel) const noexcept {
        return values[static_cast<std::size_t>(channel)];
    }
};

struct ChannelTrack {
    Channel channel = Channel::PositionX;
    std::vector<Keyframe> keys;
    bool audioEnabled = false;
    AudioModulation audio{};
};

// A choreography is evaluated from immutable base parameters every frame.
// Tracks are absolute values, audio offsets are additive. Repeated evaluation
// at the same time is deterministic, with no accumulated transform drift.
class Choreography {
public:
    std::vector<ChannelTrack> tracks;
    double durationSeconds = 0.0;
    bool loop = false;
    bool pingPong = false;

    [[nodiscard]] double localTime(double seconds) const noexcept {
        if (durationSeconds <= 0.0 || (!loop && !pingPong))
            return seconds < 0.0 ? 0.0 : seconds;
        const double t = std::fmod(std::max(0.0, seconds), durationSeconds * (pingPong ? 2.0 : 1.0));
        return pingPong && t > durationSeconds ? 2.0 * durationSeconds - t : t;
    }

    [[nodiscard]] AnimatedValues evaluate(double seconds,
                                          const AnimatedValues& base,
                                          const AudioFeatures& features) const noexcept {
        AnimatedValues result = base;
        const double t = localTime(seconds);
        for (const auto& track : tracks) {
            const auto index = static_cast<std::size_t>(track.channel);
            if (index >= result.values.size()) continue;
            const float value = sampleTrack(track.keys, t, base.values[index]);
            result.values[index] = value +
                (track.audioEnabled ? evaluateModulation(features, track.audio) : 0.f);
        }
        return result;
    }
};

} // namespace voxen::animation
