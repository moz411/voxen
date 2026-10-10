#pragma once
#include <array>
#include <cstddef>
#include <cstdint>

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

    Scene() : artworks_{{
        {1, ArtworkKind::NeonCube, {{-1.05F, 0.0F, -1.5F}, 0.34F, 0.0F}, 1.0F},
        {2, ArtworkKind::OrganicSphere, {{-0.35F, 0.0F, -1.5F}, 0.31F, 0.0F}, 1.0F},
        {3, ArtworkKind::OrganicTorus, {{0.35F, 0.0F, -1.5F}, 0.31F, 0.0F}, 1.0F},
        {4, ArtworkKind::Fractal, {{1.05F, 0.0F, -1.5F}, 0.45F, 0.0F}, 1.0F},
    }} {}

    [[nodiscard]] const std::array<Artwork, capacity>& artworks() const noexcept { return artworks_; }
    [[nodiscard]] std::array<Artwork, capacity>& artworks() noexcept { return artworks_; }
    [[nodiscard]] const Artwork* find(std::uint32_t id) const noexcept {
        for (const auto& item : artworks_) if (item.id == id) return &item;
        return nullptr;
    }
    [[nodiscard]] Artwork* find(std::uint32_t id) noexcept {
        for (auto& item : artworks_) if (item.id == id) return &item;
        return nullptr;
    }

private:
    std::array<Artwork, capacity> artworks_;
};

} // namespace voxen::scene
