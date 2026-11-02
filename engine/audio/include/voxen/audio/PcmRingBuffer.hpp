#pragma once
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <algorithm>

namespace voxen::audio {

// Single-producer/single-consumer PCM queue. Capacity is a power of two.
// Producer: decoder thread. Consumer: AAudio callback. No locks or allocations.
template <std::size_t Capacity = 16384>
class PcmRingBuffer {
    static_assert(Capacity > 1 && (Capacity & (Capacity - 1)) == 0);
public:
    [[nodiscard]] std::size_t write(const float* source, std::size_t count) noexcept {
        if (!source) return 0;
        const auto head = head_.load(std::memory_order_relaxed);
        const auto tail = tail_.load(std::memory_order_acquire);
        const std::size_t available = Capacity - (head - tail);
        const auto n = std::min(count, available);
        for (std::size_t i = 0; i < n; ++i)
            data_[(head + i) & (Capacity - 1)] = source[i];
        head_.store(head + n, std::memory_order_release);
        return n;
    }
    [[nodiscard]] std::size_t read(float* destination, std::size_t count) noexcept {
        if (!destination) return 0;
        const auto tail = tail_.load(std::memory_order_relaxed);
        const auto head = head_.load(std::memory_order_acquire);
        const auto n = std::min(count, head - tail);
        for (std::size_t i = 0; i < n; ++i)
            destination[i] = data_[(tail + i) & (Capacity - 1)];
        tail_.store(tail + n, std::memory_order_release);
        return n;
    }
    [[nodiscard]] std::size_t size() const noexcept {
        return head_.load(std::memory_order_acquire) -
               tail_.load(std::memory_order_acquire);
    }
    // Call only after stopping both producer and consumer.
    void resetStopped() noexcept {
        head_.store(0, std::memory_order_relaxed);
        tail_.store(0, std::memory_order_relaxed);
    }
private:
    std::array<float, Capacity> data_{};
    alignas(64) std::atomic<std::uint64_t> head_{0};
    alignas(64) std::atomic<std::uint64_t> tail_{0};
};
} // namespace voxen::audio
