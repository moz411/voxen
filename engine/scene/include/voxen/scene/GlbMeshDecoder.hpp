#pragma once
// Runtime GLB mesh decoder built around cgltf (MIT, jkuhlmann/cgltf).
// CPU-only output: scene nodes, transforms, vertices, indices, PBR factors.
// Rendering/uploads and texture decoding are separate follow-up work.
#include <array>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace voxen::scene::gltf {
struct Vertex {
    std::array<float,3> position{};
    std::array<float,3> normal{};
    std::array<float,2> uv{};
};
struct Primitive {
    std::vector<Vertex> vertices;
    std::vector<std::uint32_t> indices;
    std::array<float,4> baseColor{1,1,1,1};
    float metallic=1.f;
    float roughness=1.f;
    int materialIndex=-1;
    int baseColorTexture=-1;
    // Material textures intentionally not uploaded at this stage.
};
struct Node {
    std::string name;
    int parent=-1;
    std::array<float,16> worldMatrix{};
    std::array<float,3> translation{0,0,0};
    std::array<float,4> rotation{0,0,0,1};
    std::array<float,3> scale{1,1,1};
    std::vector<std::uint32_t> primitives;
};
enum class AnimationPath { Translation, Rotation, Scale };
struct AnimationChannel {
    int node=-1;
    AnimationPath path=AnimationPath::Translation;
    bool step=false;
    std::vector<float> times;
    std::vector<std::array<float,4>> values;
};
struct AnimationClip {
    std::string name;
    float duration=0.f;
    std::vector<AnimationChannel> channels;
};
struct EmbeddedImage {
    std::vector<std::uint8_t> encoded;
    std::string mimeType;
    std::vector<std::uint8_t> rgba;
    int width=0, height=0;
};
struct MeshAsset {
    std::vector<Primitive> primitives;
    std::vector<Node> nodes;
    std::vector<AnimationClip> animations;
    std::vector<EmbeddedImage> images;
};
struct Limits {
    std::size_t maxFileBytes=64u*1024u*1024u;
    std::size_t maxVertices=2'000'000;
    std::size_t maxIndices=6'000'000;
    std::size_t maxNodes=2048;
    std::size_t maxEmbeddedImageBytes=32u*1024u*1024u;
};
MeshAsset decodeMeshGlb(const std::vector<std::uint8_t>& bytes, const Limits& limits={});
MeshAsset loadMeshGlb(const std::filesystem::path& path, const Limits& limits={});
} // namespace voxen::scene::gltf
