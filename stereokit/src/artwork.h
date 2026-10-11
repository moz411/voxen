#pragma once
#include <stereokit.h>
#include <cstdint>
#include <vector>

namespace voxen {
// An Artwork is a semantic, editable unit. Voxel storage remains contiguous
// for now; first_voxel/count identify a contiguous interval until GPU batching.
struct Artwork {
    uint64_t id = 0;
    uint32_t first_voxel = 0;
    uint32_t voxel_count = 0;
    int generator = -1;       // -1 = freehand
    int shape = 0;
    int animation = 0;
    int shader = 0;
    int sound = 0;
    sk::vec3 origin{};
    float extent = 0.0f;
};
// Snapshot for an undoable operation. Metadata and voxels are both restored.
template<class Voxel> struct ArtworkSnapshot {
    Artwork artwork;
    std::vector<Voxel> voxels;
};
} // namespace voxen
