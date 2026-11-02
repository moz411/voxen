#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace voxen::animation {

// Audio analysis is produced by the playback/decoder thread and consumed by the
// scene update thread. This is a value snapshot, not a shared mutable buffer.
struct AudioFeatures {
    double timestampSeconds = 0.0;
    float rms = 0.0f;
    float bass = 0.0f;
    float mid = 0.0f;
    float treble = 0.0f;
    float onset = 0.0f;
};

enum class Interpolation : std::uint8_t { Step, Linear, Smooth };

struct Keyframe {
    double timeSeconds = 0.0;
    float value = 0.0f;
    Interpolation interpolation = Interpolation::Linear;
};

inline float sampleTrack(const std::vector<Keyframe>& keys, double timeSeconds,
                         float fallback = 0.0f) {
    if (keys.empty()) return fallback;
    if (timeSeconds <= keys.front().timeSeconds) return keys.front().value;
    if (timeSeconds >= keys.back().timeSeconds) return keys.back().value;
    const auto next = std::upper_bound(keys.begin(), keys.end(), timeSeconds,
        [](double t, const Keyframe& key) { return t < key.timeSeconds; });
    const Keyframe& b = *next;
    const Keyframe& a = *(next - 1);
    const double duration = b.timeSeconds - a.timeSeconds;
    if (duration <= 0.0 || a.interpolation == Interpolation::Step) return a.value;
    float t = static_cast<float>((timeSeconds - a.timeSeconds) / duration);
    t = std::clamp(t, 0.0f, 1.0f);
    if (a.interpolation == Interpolation::Smooth) t = t * t * (3.0f - 2.0f * t);
    return a.value + (b.value - a.value) * t;
}

enum class AudioBand : std::uint8_t { Rms, Bass, Mid, Treble, Onset };

inline float bandValue(const AudioFeatures& features, AudioBand band) {
    switch (band) {
        case AudioBand::Rms: return features.rms;
        case AudioBand::Bass: return features.bass;
        case AudioBand::Mid: return features.mid;
        case AudioBand::Treble: return features.treble;
        case AudioBand::Onset: return features.onset;
    }
    return 0.0f;
}

struct AudioModulation {
    AudioBand band = AudioBand::Bass;
    float gain = 1.0f;
    float offset = 0.0f;
    float minimum = -1.0f;
    float maximum = 1.0f;
};

inline float evaluateModulation(const AudioFeatures& features,
                                const AudioModulation& mapping) {
    return std::clamp(mapping.offset + mapping.gain * bandValue(features, mapping.band),
                      mapping.minimum, mapping.maximum);
}

// Keep base values immutable: timeline sets an absolute animated value;
// audio modulation contributes an additive offset.
inline float composeParameter(float base, const std::vector<Keyframe>& track,
                              double seconds, const AudioFeatures& audio,
                              const AudioModulation& modulation) {
    return sampleTrack(track, seconds, base) + evaluateModulation(audio, modulation);
}

} // namespace voxen::animation
