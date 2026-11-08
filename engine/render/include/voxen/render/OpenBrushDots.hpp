#pragma once
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace voxen::render::openbrush {
// Vulkan instance vertex attributes (locations 2..4 in openbrush_dots.vert).
// No Vulkan headers required: usable in host-side unit tests.
struct DotInstance {
    std::array<float, 3> center{};
    std::array<float, 4> color{1.f, 1.f, 1.f, 1.f};
    float diameterMeters{0.01f};
};
struct ControlPoint {
    std::array<float, 3> position{};
    float pressure{1.f};
};
inline std::vector<DotInstance> buildDots(
    const std::vector<ControlPoint>& points,
    const std::array<float, 4>& color,
    float brushSizeMeters,
    float brushScale = 1.f,
    float worldScale = 1.f) {
    std::vector<DotInstance> instances;
    instances.reserve(points.size());
    if (!std::isfinite(brushSizeMeters) || !std::isfinite(brushScale) ||
        !std::isfinite(worldScale) || brushSizeMeters <= 0.f ||
        brushScale <= 0.f || worldScale <= 0.f) return instances;
    for (const auto& point : points) {
        bool finite = std::isfinite(point.pressure);
        for (float v : point.position) finite = finite && std::isfinite(v);
        if (!finite) continue;
        DotInstance dot;
        for (int i = 0; i < 3; ++i) dot.center[i] = point.position[i] * worldScale;
        dot.color = color;
        dot.diameterMeters = brushSizeMeters * brushScale * worldScale *
                             std::clamp(point.pressure, 0.f, 1.f);
        if (std::isfinite(dot.diameterMeters) && dot.diameterMeters > 0.f)
            instances.push_back(dot);
    }
    return instances;
}
} // namespace voxen::render::openbrush
