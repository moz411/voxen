#pragma once
#include <array>
#include <cstdint>
#include <vector>
#include <string>
#include <utility>

namespace voxen::scene {

// Neutral data contract for Meta OpenXR scene capture today and optional
// ARCore/mobile providers in the future. Coordinates are OpenXR LOCAL-space
// meters, never controller-relative and never guessed from passthrough pixels.
enum class SurfaceClass : std::uint8_t { Unknown, Wall, Floor, Ceiling };
struct EnvironmentSurface {
    std::uint64_t id = 0;
    SurfaceClass classification = SurfaceClass::Unknown;
    std::array<float, 3> center{0.f,0.f,0.f};
    // Physical half-extents, in meters; normal is local +Z.
    std::array<float, 2> halfExtents{0.5f,0.5f};
    // Rotation quaternion (x,y,z,w), unit-length.
    std::array<float, 4> orientation{0.f,0.f,0.f,1.f};
    bool tracked = false;
    // Stable Meta spatial-entity UUID, never an XrSpace handle or LOCAL pose.
    // Empty for providers that cannot guarantee persistent identity.
    std::string persistentUuid;
};

class EnvironmentSurfaceRegistry {
public:
    void replace(std::vector<EnvironmentSurface> surfaces) {
        surfaces_ = std::move(surfaces);
    }
    void clear() noexcept { surfaces_.clear(); }
    [[nodiscard]] const std::vector<EnvironmentSurface>& surfaces() const noexcept {
        return surfaces_;
    }
private:
    std::vector<EnvironmentSurface> surfaces_;
};
} // namespace voxen::scene
