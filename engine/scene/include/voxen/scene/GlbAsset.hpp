#pragma once
// GLB 2.0 container reader. No GPU allocations; safe to run before Vulkan upload.
// This is the input/validation layer, not yet a glTF mesh/material renderer.
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace voxen::scene::gltf {
struct GlbAsset {
    std::string json;
    std::vector<std::uint8_t> binary;
};

inline std::uint32_t u32le(const std::uint8_t* p) noexcept {
    return std::uint32_t(p[0]) | (std::uint32_t(p[1]) << 8) |
           (std::uint32_t(p[2]) << 16) | (std::uint32_t(p[3]) << 24);
}

// The upper bound is on the encoded input, not decompressed GPU memory.
// Asset policy can impose lower, scene-specific budgets.
inline GlbAsset decodeGlb(const std::vector<std::uint8_t>& data,
                          std::size_t maxBytes = 64u * 1024u * 1024u) {
    constexpr std::uint32_t magic = 0x46546c67u; // 'glTF'
    constexpr std::uint32_t jsonType = 0x4e4f534au; // 'JSON'
    constexpr std::uint32_t binType = 0x004e4942u;  // 'BIN\0'
    if (data.size() < 20 || data.size() > maxBytes)
        throw std::runtime_error("GLB: invalid size or size budget exceeded");
    if (u32le(data.data()) != magic || u32le(data.data() + 4) != 2 ||
        u32le(data.data() + 8) != data.size())
        throw std::runtime_error("GLB: bad header, version or declared length");

    std::size_t pos = 12;
    bool jsonSeen = false, binSeen = false;
    GlbAsset result;
    while (pos < data.size()) {
        if (data.size() - pos < 8)
            throw std::runtime_error("GLB: truncated chunk header");
        const auto length = u32le(data.data() + pos);
        const auto type = u32le(data.data() + pos + 4);
        pos += 8;
        if ((length & 3u) != 0 || length > data.size() - pos)
            throw std::runtime_error("GLB: invalid chunk length");
        if (!jsonSeen) {
            if (type != jsonType || length == 0)
                throw std::runtime_error("GLB: JSON must be the first nonempty chunk");
            result.json.assign(reinterpret_cast<const char*>(data.data() + pos), length);
            // JSON chunks are padded with ASCII spaces; remove only padding.
            while (!result.json.empty() && result.json.back() == ' ')
                result.json.pop_back();
            if (result.json.empty() || result.json.front() != '{' ||
                result.json.back() != '}')
                throw std::runtime_error("GLB: malformed JSON chunk envelope");
            jsonSeen = true;
        } else if (type == binType) {
            if (binSeen)
                throw std::runtime_error("GLB: multiple BIN chunks");
            result.binary.assign(data.begin() + static_cast<std::ptrdiff_t>(pos),
                                 data.begin() + static_cast<std::ptrdiff_t>(pos + length));
            binSeen = true;
        }
        // Unknown extension chunks are allowed by GLB 2.0 and safely skipped.
        pos += length;
    }
    if (!jsonSeen)
        throw std::runtime_error("GLB: missing JSON chunk");
    return result;
}

inline GlbAsset loadGlb(const std::filesystem::path& file,
                        std::size_t maxBytes = 64u * 1024u * 1024u) {
    std::ifstream stream(file, std::ios::binary | std::ios::ate);
    if (!stream) throw std::runtime_error("GLB: cannot open file");
    const auto end = stream.tellg();
    if (end < 0 || static_cast<std::uintmax_t>(end) > maxBytes ||
        static_cast<std::uintmax_t>(end) > std::numeric_limits<std::size_t>::max())
        throw std::runtime_error("GLB: file exceeds size budget");
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(end));
    stream.seekg(0);
    if (!bytes.empty() && !stream.read(reinterpret_cast<char*>(bytes.data()),
                                       static_cast<std::streamsize>(bytes.size())))
        throw std::runtime_error("GLB: truncated file");
    return decodeGlb(bytes, maxBytes);
}
} // namespace voxen::scene::gltf
