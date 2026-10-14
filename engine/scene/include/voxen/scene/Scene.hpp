#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace voxen::scene {

enum class ArtworkKind : std::uint8_t { NeonCube, OrganicSphere, OrganicTorus, Fractal };

struct Transform {
    std::array<float, 3> position{};
    float scale = 1.0F;
    float yawRadians = 0.0F;
};

struct Artwork {
    std::uint32_t id = 0;
    ArtworkKind kind = ArtworkKind::NeonCube;
    Transform transform{};
    float animationSpeed = 1.0F;
};

class Scene {
public:
    static constexpr std::size_t capacity = 4;

    Scene() = default; // Start in an empty MR workspace.

    [[nodiscard]] const std::vector<Artwork>& artworks() const noexcept { return artworks_; }
    [[nodiscard]] std::vector<Artwork>& artworks() noexcept { return artworks_; }
    [[nodiscard]] const Artwork* find(std::uint32_t id) const noexcept {
        for (const auto& item : artworks_) if (item.id == id) return &item;
        return nullptr;
    }
    [[nodiscard]] Artwork* find(std::uint32_t id) noexcept {
        for (auto& item : artworks_) if (item.id == id) return &item;
        return nullptr;
    }

private:
    std::vector<Artwork> artworks_;
};

} // namespace voxen::scene
