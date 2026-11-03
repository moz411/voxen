#pragma once
#include <voxen/audio/SpectrumAnalyzer.hpp>
#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace voxen::audio {
// Background-only MP3 analysis. MediaPlayer remains the audible playback source;
// features are indexed by its playback position (no audio callback work).
class Mp3FeatureTimeline {
public:
    Mp3FeatureTimeline() = default;
    ~Mp3FeatureTimeline() { stop(); }
    Mp3FeatureTimeline(const Mp3FeatureTimeline&) = delete;
    Mp3FeatureTimeline& operator=(const Mp3FeatureTimeline&) = delete;
    void start(const std::string& path);
    void stop();
    [[nodiscard]] SpectrumFeatures at(double seconds) const;
    [[nodiscard]] bool ready() const noexcept { return ready_.load(); }
private:
    struct Frame { double seconds; SpectrumFeatures features; };
    void decode(std::stop_token token, std::string path);
    mutable std::mutex mutex_;
    std::vector<Frame> frames_;
    std::jthread worker_;
    std::atomic<bool> ready_{false};
};
} // namespace voxen::audio
