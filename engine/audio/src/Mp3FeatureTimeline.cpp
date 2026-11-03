#include <voxen/audio/Mp3FeatureTimeline.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <utility>
#ifdef __ANDROID__
#include <media/NdkMediaCodec.h>
#include <media/NdkMediaExtractor.h>
#include <media/NdkMediaFormat.h>
#endif

namespace voxen::audio {
void Mp3FeatureTimeline::stop() {
    if (worker_.joinable()) {
        worker_.request_stop();
        worker_.join();
    }
    std::lock_guard lock(mutex_);
    frames_.clear();
    ready_.store(false);
}
void Mp3FeatureTimeline::start(const std::string& path) {
    stop();
    if (path.empty()) return;
    worker_ = std::jthread([this, path](std::stop_token token) { decode(token, path); });
}
SpectrumFeatures Mp3FeatureTimeline::at(double seconds) const {
    if (!ready_.load(std::memory_order_acquire) || !std::isfinite(seconds) || seconds < 0.0) return {};
    std::lock_guard lock(mutex_);
    if (frames_.empty()) return {};
    const auto it = std::lower_bound(frames_.begin(), frames_.end(), seconds,
        [](const Frame& f, double t) { return f.seconds < t; });
    if (it == frames_.begin()) return it->features;
    if (it == frames_.end()) return frames_.back().features;
    const auto& a = *(it - 1);
    const auto& b = *it;
    const float w = static_cast<float>(std::clamp((seconds - a.seconds) /
        std::max(0.001, b.seconds - a.seconds), 0.0, 1.0));
    return {a.features.rms + (b.features.rms - a.features.rms) * w,
            a.features.bass + (b.features.bass - a.features.bass) * w,
            a.features.mid + (b.features.mid - a.features.mid) * w,
            a.features.treble + (b.features.treble - a.features.treble) * w,
            a.features.onset + (b.features.onset - a.features.onset) * w};
}
void Mp3FeatureTimeline::decode(std::stop_token token, std::string path) {
#ifdef __ANDROID__
    AMediaExtractor* extractor = AMediaExtractor_new();
    if (!extractor) return;
    if (AMediaExtractor_setDataSource(extractor, path.c_str()) != AMEDIA_OK) {
        AMediaExtractor_delete(extractor); return;
    }
    AMediaCodec* codec = nullptr;
    int32_t rate = 0, channels = 0;
    for (size_t i = 0; i < AMediaExtractor_getTrackCount(extractor); ++i) {
        AMediaFormat* format = AMediaExtractor_getTrackFormat(extractor, i);
        const char* mime = nullptr;
        const bool audio = format && AMediaFormat_getString(format, AMEDIAFORMAT_KEY_MIME, &mime)
            && mime && std::string(mime).starts_with("audio/");
        if (audio && AMediaFormat_getInt32(format, AMEDIAFORMAT_KEY_SAMPLE_RATE, &rate)
                  && AMediaFormat_getInt32(format, AMEDIAFORMAT_KEY_CHANNEL_COUNT, &channels)
                  && rate > 0 && channels > 0 && channels <= 8) {
            codec = AMediaCodec_createDecoderByType(mime);
            if (codec && AMediaCodec_configure(codec, format, nullptr, nullptr, 0) == AMEDIA_OK
                      && AMediaCodec_start(codec) == AMEDIA_OK) {
                AMediaExtractor_selectTrack(extractor, i);
                AMediaFormat_delete(format);
                break;
            }
            if (codec) { AMediaCodec_delete(codec); codec = nullptr; }
        }
        if (format) AMediaFormat_delete(format);
    }
    if (!codec) { AMediaExtractor_delete(extractor); return; }
    SpectrumAnalyzer analyzer;
    std::array<float, SpectrumAnalyzer::Window> window{};
    size_t filled = 0;
    uint64_t sampleIndex = 0;
    bool inputEnd = false, outputEnd = false;
    std::vector<Frame> result;
    // One feature point per 1024 samples (~21 ms at 48 kHz).
    // Cap analysis at two hours to bound memory for malformed media.
    const uint64_t maxSamples = static_cast<uint64_t>(rate) * 7200u;
    while (!token.stop_requested() && !outputEnd && sampleIndex < maxSamples) {
        if (!inputEnd) {
            const ssize_t index = AMediaCodec_dequeueInputBuffer(codec, 1000);
            if (index >= 0) {
                size_t capacity = 0;
                uint8_t* buffer = AMediaCodec_getInputBuffer(codec, index, &capacity);
                const ssize_t size = buffer ? AMediaExtractor_readSampleData(extractor, buffer, capacity) : -1;
                if (size < 0) {
                    AMediaCodec_queueInputBuffer(codec, index, 0, 0, 0, AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM);
                    inputEnd = true;
                } else {
                    const int64_t pts = AMediaExtractor_getSampleTime(extractor);
                    AMediaCodec_queueInputBuffer(codec, index, 0, static_cast<size_t>(size), pts, 0);
                    AMediaExtractor_advance(extractor);
                }
            }
        }
        AMediaCodecBufferInfo info{};
        const ssize_t index = AMediaCodec_dequeueOutputBuffer(codec, &info, 1000);
        if (index == AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED) {
            AMediaFormat* output = AMediaCodec_getOutputFormat(codec);
            int32_t newRate = rate, newChannels = channels;
            AMediaFormat_getInt32(output, AMEDIAFORMAT_KEY_SAMPLE_RATE, &newRate);
            AMediaFormat_getInt32(output, AMEDIAFORMAT_KEY_CHANNEL_COUNT, &newChannels);
            if (newRate > 0) rate = newRate;
            if (newChannels > 0 && newChannels <= 8) channels = newChannels;
            AMediaFormat_delete(output);
        } else if (index >= 0) {
            size_t capacity = 0;
            const uint8_t* data = AMediaCodec_getOutputBuffer(codec, index, &capacity);
            // Android decoders normally output interleaved signed 16-bit PCM.
            // Do not interpret float/other encodings as int16.
            AMediaFormat* output = AMediaCodec_getOutputFormat(codec);
            int32_t encoding = 2; // ENCODING_PCM_16BIT
            if (output) {
                AMediaFormat_getInt32(output, "pcm-encoding", &encoding);
                AMediaFormat_delete(output);
            }
            if (data && encoding == 2 && info.offset >= 0 && info.size > 0 &&
                static_cast<size_t>(info.offset) + static_cast<size_t>(info.size) <= capacity) {
                const uint8_t* bytes = data + info.offset;
                const size_t count = static_cast<size_t>(info.size) / (2u * static_cast<size_t>(channels));
                for (size_t n = 0; n < count && sampleIndex < maxSamples; ++n) {
                    float mono = 0.f;
                    for (int ch = 0; ch < channels; ++ch) {
                        const size_t offset = (n * channels + ch) * 2;
                        const int16_t sample = static_cast<int16_t>(
                            static_cast<uint16_t>(bytes[offset]) |
                            (static_cast<uint16_t>(bytes[offset + 1]) << 8));
                        mono += static_cast<float>(sample) / 32768.f;
                    }
                    window[filled++] = mono / static_cast<float>(channels);
                    ++sampleIndex;
                    if (filled == window.size()) {
                        result.push_back({static_cast<double>(sampleIndex - window.size()/2) / rate,
                            analyzer.analyze(window.data(), window.size(), 1, rate)});
                        filled = 0;
                        // Publish incrementally so playback reacts before the
                        // entire MP3 has been decoded. Never wait for EOF.
                        if (result.size() >= 256) {
                            std::lock_guard lock(mutex_);
                            frames_.insert(frames_.end(), result.begin(), result.end());
                            result.clear();
                            ready_.store(true, std::memory_order_release);
                        }
                    }
                }
            }
            outputEnd = (info.flags & AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM) != 0;
            AMediaCodec_releaseOutputBuffer(codec, index, false);
        }
    }
    AMediaCodec_stop(codec);
    AMediaCodec_delete(codec);
    AMediaExtractor_delete(extractor);
    if (!token.stop_requested() && !result.empty()) {
        std::lock_guard lock(mutex_);
        frames_.insert(frames_.end(), result.begin(), result.end());
        ready_.store(true, std::memory_order_release);
    }
#else
    (void)token; (void)path;
#endif
}
} // namespace voxen::audio
