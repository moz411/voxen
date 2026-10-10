#pragma once
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <unordered_set>
#include <vector>

namespace voxen::voxel {
struct Cell {
    int x = 0, y = 0, z = 0;
    bool operator==(const Cell&) const = default;
};
struct CellHash {
    std::size_t operator()(const Cell& c) const noexcept {
        std::uint64_t h = 1469598103934665603ull;
        for (int v : {c.x, c.y, c.z}) {
            h ^= static_cast<std::uint32_t>(v);
            h *= 1099511628211ull;
        }
        return static_cast<std::size_t>(h);
    }
};

class VoxelWorld {
public:
    static constexpr float cellSize = 0.05F;
    static constexpr std::size_t maxCells = 2048;
    [[nodiscard]] const std::vector<Cell>& cells() const noexcept { return cells_; }
    [[nodiscard]] static Cell quantize(const std::array<float,3>& p) noexcept {
        return {static_cast<int>(std::floor(p[0]/cellSize)),
                static_cast<int>(std::floor(p[1]/cellSize)),
                static_cast<int>(std::floor(p[2]/cellSize))};
    }
    bool add(const Cell& c) {
        if (cells_.size() >= maxCells || !occupied_.insert(c).second) return false;
        cells_.push_back(c);
        return true;
    }
    void stroke(const std::array<float,3>& from, const std::array<float,3>& to) {
        const float dx=to[0]-from[0], dy=to[1]-from[1], dz=to[2]-from[2];
        const float distance=std::sqrt(dx*dx+dy*dy+dz*dz);
        const int steps=static_cast<int>(std::ceil(distance/(cellSize*0.4F)));
        // Cap interpolation on tracking jumps; a new stroke starts after an interruption.
        if (steps > 200) { add(quantize(to)); return; }
        for (int i=0; i<=(steps > 0 ? steps : 1); ++i) {
            const float t=steps > 0 ? static_cast<float>(i)/steps : 1.0F;
            add(quantize({from[0]+dx*t, from[1]+dy*t, from[2]+dz*t}));
        }
    }
private:
    std::vector<Cell> cells_;
    std::unordered_set<Cell, CellHash> occupied_;
};
} // namespace voxen::voxel
