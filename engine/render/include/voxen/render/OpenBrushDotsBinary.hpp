#pragma once
#include <voxen/render/OpenBrushDots.hpp>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <vector>

namespace voxen::render::openbrush {
inline std::uint32_t readU32LE(const std::uint8_t* p) {
    return std::uint32_t(p[0]) | (std::uint32_t(p[1]) << 8) |
           (std::uint32_t(p[2]) << 16) | (std::uint32_t(p[3]) << 24);
}
inline float readF32LE(const std::uint8_t* p) {
    const auto bits = readU32LE(p);
    float value;
    static_assert(sizeof(value) == sizeof(bits));
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}
inline std::vector<DotInstance> decodeDotsBinary(const std::uint8_t* bytes,
                                                 std::size_t size) {
    constexpr std::uint8_t magic[8] = {'V','X','D','O','T','0','0','1'};
    if (!bytes || size < 16 || std::memcmp(bytes, magic, 8) != 0)
        throw std::runtime_error("Invalid Dots payload magic");
    const auto count = readU32LE(bytes + 8);
    const auto stride = readU32LE(bytes + 12);
    if (stride != 32 || count > (size - 16) / 32 ||
        size != 16 + std::size_t(count) * 32)
        throw std::runtime_error("Invalid Dots payload length");
    std::vector<DotInstance> result;
    result.reserve(count);
    for (std::uint32_t i = 0; i < count; ++i) {
        const auto* p = bytes + 16 + std::size_t(i) * 32;
        DotInstance dot;
        for (int j = 0; j < 3; ++j) dot.center[j] = readF32LE(p + j * 4);
        for (int j = 0; j < 4; ++j) dot.color[j] = readF32LE(p + (j + 3) * 4);
        dot.diameterMeters = readF32LE(p + 28);
        result.push_back(dot);
    }
    return result;
}
} // namespace voxen::render::openbrush
