#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace voxen::audio {

// Run on decoded PCM outside the real-time AAudio callback. No allocations.
// Input is interleaved float PCM; bands are normalized energy estimates.
struct SpectrumFeatures {
    float rms = 0.f, bass = 0.f, mid = 0.f, treble = 0.f, onset = 0.f;
};

class SpectrumAnalyzer {
public:
    static constexpr std::size_t Window = 1024;
    SpectrumFeatures analyze(const float* pcm, std::size_t frames,
                             unsigned channels, unsigned sampleRate) noexcept {
        if (!pcm || frames < Window || channels == 0 || sampleRate == 0)
            return {};
        std::array<float, Window> samples{};
        double power = 0.0;
        for (std::size_t i = 0; i < Window; ++i) {
            float s = 0.f;
            for (unsigned ch = 0; ch < channels; ++ch)
                s += pcm[i * channels + ch];
            s /= static_cast<float>(channels);
            power += static_cast<double>(s) * s;
            samples[i] = s * (0.5f - 0.5f * std::cos(
                6.283185307179586f * static_cast<float>(i) / (Window - 1)));
        }
        // In-place iterative radix-2 FFT, no complex heap storage.
        std::array<float, Window> imag{};
        for (std::size_t i = 1, j = 0; i < Window; ++i) {
            std::size_t bit = Window >> 1;
            for (; j & bit; bit >>= 1) j ^= bit;
            j ^= bit;
            if (i < j) {
                std::swap(samples[i], samples[j]);
                std::swap(imag[i], imag[j]);
            }
        }
        for (std::size_t len = 2; len <= Window; len <<= 1) {
            const float angle = -6.283185307179586f / static_cast<float>(len);
            const float wr = std::cos(angle), wi = std::sin(angle);
            for (std::size_t start = 0; start < Window; start += len) {
                float cr = 1.f, ci = 0.f;
                for (std::size_t j = 0; j < len / 2; ++j) {
                    const std::size_t a = start + j, b = a + len / 2;
                    const float tr = cr * samples[b] - ci * imag[b];
                    const float ti = cr * imag[b] + ci * samples[b];
                    samples[b] = samples[a] - tr;
                    imag[b] = imag[a] - ti;
                    samples[a] += tr;
                    imag[a] += ti;
                    const float nr = cr * wr - ci * wi;
                    ci = cr * wi; cr = nr;
                }
            }
        }
        double bands[3]{};
        for (std::size_t k = 1; k < Window / 2; ++k) {
            const double hz = static_cast<double>(k) * sampleRate / Window;
            const unsigned band = hz < 250. ? 0 : hz < 2000. ? 1 : 2;
            bands[band] += static_cast<double>(samples[k]) * samples[k] +
                           static_cast<double>(imag[k]) * imag[k];
        }
        SpectrumFeatures result;
        result.rms = static_cast<float>(std::sqrt(power / Window));
        result.bass = static_cast<float>(std::sqrt(bands[0]) / Window);
        result.mid = static_cast<float>(std::sqrt(bands[1]) / Window);
        result.treble = static_cast<float>(std::sqrt(bands[2]) / Window);
        const float energy = result.bass + result.mid + result.treble;
        result.onset = std::max(0.f, energy - previousEnergy_);
        previousEnergy_ = energy;
        return result;
    }
    void reset() noexcept { previousEnergy_ = 0.f; }
private:
    float previousEnergy_ = 0.f;
};

} // namespace voxen::audio
