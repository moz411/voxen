#pragma once
#include <array>
#include <cstdint>
#include <iomanip>
#include <random>
#include <sstream>
#include <string>

namespace voxen::gallery {

// UUID v4 for durable scene and placement identifiers. Generate once when an
// object is created; never regenerate during load or anchor relocalization.
inline std::string newUuid() {
    std::random_device entropy;
    std::mt19937_64 rng(entropy());
    std::uniform_int_distribution<std::uint32_t> byte(0, 255);
    std::array<std::uint8_t, 16> b{};
    for (auto& v : b) v = static_cast<std::uint8_t>(byte(rng));
    b[6] = static_cast<std::uint8_t>((b[6] & 0x0f) | 0x40);
    b[8] = static_cast<std::uint8_t>((b[8] & 0x3f) | 0x80);
    std::ostringstream out;
    out << std::hex << std::setfill('0');
    for (std::size_t i = 0; i < b.size(); ++i) {
        if (i == 4 || i == 6 || i == 8 || i == 10) out << '-';
        out << std::setw(2) << static_cast<unsigned>(b[i]);
    }
    return out.str();
}
} // namespace voxen::gallery
