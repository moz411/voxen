#pragma once
// Deterministic native Embers: static GPU buffers, animated in vertex shader.
#include <voxen/render/OpenBrushSmokeStroke.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>
namespace voxen::render::openbrush {
inline constexpr int EmbersStrokePreset=31;
inline CompiledScene makeEmbersStrokeScene(const std::vector<SmokeStrokePoint>& points) {
    CompiledScene scene;
    scene.formatVersion=2;
    if(points.empty())return scene;
    CompiledMesh mesh;
    mesh.brushGuid="voxen-native-embers";
    for(std::size_t i=0;i<points.size();++i) {
        const auto& p=points[i];
        for(int n=0;n<7;++n) {
            const float phase=float(i*43+n*29)*2.39996323f;
            const float spread=p.size*0.45f;
            const std::array<float,3> center{
                p.center[0]+std::sin(phase)*spread,
                p.center[1]+std::cos(phase*1.2f)*spread,
                p.center[2]+std::sin(phase*0.8f)*spread};
            const float radius=p.size*(0.012f+0.004f*float(n%3));
            // Crossed diamond-shaped glowing motes.
            for(int plane=0;plane<2;++plane) {
                const std::uint32_t base=static_cast<std::uint32_t>(mesh.positions.size()/3);
                const std::array<std::array<float,2>,4> corners{{{0.f,1.f},{1.f,0.f},{0.f,-1.f},{-1.f,0.f}}};
                for(const auto& c:corners) {
                    mesh.positions.insert(mesh.positions.end(),{
                        center[0]+(plane?0.f:c[0]*radius),
                        center[1]+c[1]*radius*1.6f,
                        center[2]+(plane?c[0]*radius:0.f)});
                    mesh.normals.insert(mesh.normals.end(),center.begin(),center.end());
                    mesh.colors.insert(mesh.colors.end(),{
                        std::clamp(p.rgb[0],0.f,1.f),
                        std::clamp(p.rgb[1],0.f,1.f),
                        std::clamp(p.rgb[2],0.f,1.f),0.85f});
                    mesh.uv0.insert(mesh.uv0.end(),{phase,radius,0.f,0.f});
                }
                mesh.indices.insert(mesh.indices.end(),{
                    base,base+1,base+2,base,base+2,base+3});
            }
        }
    }
    scene.meshes.push_back(std::move(mesh));
    return scene;
}
} // namespace voxen::render::openbrush
