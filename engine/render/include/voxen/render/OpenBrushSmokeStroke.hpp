#pragma once
// Native Smoke stroke sampling/geometry, independent of Vulkan and OpenXR.
#include <voxen/render/OpenBrushCompiledScene.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace voxen::render::openbrush {
inline constexpr int SmokeStrokePreset=27; // Separate from obsolete emitters 20..26.
inline constexpr const char* SmokeBrushGuid="70d79cca-b159-4f35-990c-f02193947fe8";
struct SmokeStrokePoint {
    std::array<float,3> center{};
    float size=0.18f;
    float rotation=0.f;
    float opacity=1.f;
    std::array<float,3> rgb{0.12f,0.8f,1.f};
};
inline bool shouldSampleSmoke(const SmokeStrokePoint& previous,
                              const SmokeStrokePoint& next) {
    const float dx=next.center[0]-previous.center[0];
    const float dy=next.center[1]-previous.center[1];
    const float dz=next.center[2]-previous.center[2];
    const float spacing=std::max(0.008f,next.size*0.12f);
    return dx*dx+dy*dy+dz*dz>=spacing*spacing;
}
inline CompiledScene makeSmokeStrokeScene(const std::vector<SmokeStrokePoint>& points) {
    CompiledScene scene;
    scene.formatVersion=2;
    if(points.empty())return scene;
    CompiledMesh mesh;
    mesh.brushGuid=SmokeBrushGuid;
    const std::uint32_t count=static_cast<std::uint32_t>(points.size());
    mesh.positions.reserve(std::size_t(count)*12);
    mesh.normals.reserve(std::size_t(count)*12);
    mesh.colors.reserve(std::size_t(count)*16);
    mesh.uv0.reserve(std::size_t(count)*16);
    mesh.indices.reserve(std::size_t(count)*6);
    for(std::uint32_t i=0;i<count;++i) {
        const auto& p=points[i];
        const auto& rgb=p.rgb;
        const float h=p.size*0.5f;
        const float corners[4][2]={{-1.f,-1.f},{1.f,-1.f},{1.f,1.f},{-1.f,1.f}};
        const float uv[4][2]={{0.f,1.f},{1.f,1.f},{1.f,0.f},{0.f,0.f}};
        for(int j=0;j<4;++j) {
            mesh.positions.insert(mesh.positions.end(),{p.center[0]+corners[j][0]*h,
                p.center[1]+corners[j][1]*h,p.center[2]});
            mesh.normals.insert(mesh.normals.end(),p.center.begin(),p.center.end());
            mesh.colors.insert(mesh.colors.end(),{rgb[0],rgb[1],rgb[2],p.opacity});
            mesh.uv0.insert(mesh.uv0.end(),{uv[j][0],uv[j][1],p.rotation,0.f});
        }
        const std::uint32_t b=i*4;
        mesh.indices.insert(mesh.indices.end(),{b,b+1,b+2,b,b+2,b+3});
    }
    scene.meshes.push_back(std::move(mesh));
    return scene;
}
} // namespace voxen::render::openbrush
