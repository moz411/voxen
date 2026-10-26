#pragma once
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace voxen::render {
struct PanoramaImage {
    int width=0;
    int height=0;
    std::vector<std::uint8_t> rgba;
    // Source panoramas may cover less than 360 degrees; do not assume 2:1.
    float aspectRatio() const noexcept {
        return height>0 ? static_cast<float>(width)/height : 0.f;
    }
};
// Decode a local JPEG/PNG into RGBA8 with bounded dimensions and allocation.
// Throws std::runtime_error on invalid input.
PanoramaImage decodePanoramaImage(const std::filesystem::path& file,
                                  int maxDimension=8192,
                                  std::uint64_t maxPixels=16ull*1024ull*1024ull);
}
