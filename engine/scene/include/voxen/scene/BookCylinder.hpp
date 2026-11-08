#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

namespace voxen::gallery {

// Presentation only: scene entry/exit remains controlled by the existing
// head-position walk-in state machine, never by a controller click.
struct BookCylinderStyle {
    std::string coverAsset; // Relative to app-private Books/; validated by asset loader.
    float radiusMeters = 0.40f;
    float heightMeters = 1.60f;
    std::uint32_t coverRepeats = 3;
};

struct BookCylinderVertex {
    std::array<float, 3> position{};
    std::array<float, 3> normal{};
    std::array<float, 2> uv{};
};

// Shared unit mesh: local Y is vertical, center at origin.
// Vertex UV repeats the cover around the circumference; a repeat count of 3
// avoids stretching a portrait cover across a 360-degree panorama.
// Duplicate seam vertices so mipmapping/texture wrapping is continuous.
// Triangle winding is outward-facing; use a double-sided material if needed.
inline std::vector<BookCylinderVertex> makeBookCylinderVertices(
    std::uint32_t segments = 48, std::uint32_t repeats = 3) {
    segments = std::clamp(segments, 3u, 256u);
    repeats = std::clamp(repeats, 1u, 8u);
    std::vector<BookCylinderVertex> vertices;
    vertices.reserve(static_cast<std::size_t>(segments) * 6);
    constexpr float tau = 6.2831853071795864769f;
    auto vertex = [&](std::uint32_t i, float y) {
        const float a = tau * static_cast<float>(i) / static_cast<float>(segments);
        const float x = std::cos(a), z = std::sin(a);
        return BookCylinderVertex{{x, y, z}, {x, 0.f, z},
            {static_cast<float>(repeats) * i / segments, y + 0.5f}};
    };
    for (std::uint32_t i = 0; i < segments; ++i) {
        auto a = vertex(i, 0.5f), b = vertex(i + 1, 0.5f);
        auto c = vertex(i, -0.5f), d = vertex(i + 1, -0.5f);
        vertices.insert(vertices.end(), {a, b, c, b, d, c});
    }
    return vertices;
}

} // namespace voxen::gallery
