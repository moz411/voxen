#pragma once
// Deterministic Fire brush geometry; no per-frame particle simulation.
#include <voxen/render/OpenBrushSmokeStroke.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>
namespace voxen::render::openbrush {
inline constexpr int FireStrokePreset=29;
inline CompiledScene makeFireStrokeScene(const std::vector<SmokeStrokePoint>& points) {
    CompiledScene scene;
    scene.formatVersion=2;
    if(points.empty())return scene;
    CompiledMesh mesh;
    mesh.brushGuid="voxen-native-fire";
    for(std::size_t i=0;i<points.size();++i) {
        const auto& p=points[i];
        // Two intersecting vertical ribbons, with pointed tongues and
        // orange-to-yellow vertex colors. Stable jitter prevents repetition.
        for(int tongue=0;tongue<3;++tongue) {
            const float phase=float(i*13+tongue*31)*2.39996323f;
            const float radius=p.size*(0.14f+0.035f*float(tongue));
            const float height=p.size*(0.55f+0.2f*float(tongue));
            const float cx=p.center[0]+std::sin(phase)*p.size*0.2f;
            const float cz=p.center[2]+std::cos(phase)*p.size*0.2f;
            for(int plane=0;plane<2;++plane) {
                const float dx=plane?0.f:radius;
                const float dz=plane?radius:0.f;
                const float bend=std::sin(phase*1.3f)*radius*0.6f;
                const std::uint32_t base=static_cast<std::uint32_t>(mesh.positions.size()/3);
                const std::array<std::array<float,3>,3> vertices{{
                    {cx-dx,p.center[1],cz-dz},
                    {cx+dx,p.center[1],cz+dz},
                    {cx+bend,p.center[1]+height,cz+bend}
                }};
                for(int j=0;j<3;++j) {
                    mesh.positions.insert(mesh.positions.end(),vertices[j].begin(),vertices[j].end());
                    mesh.normals.insert(mesh.normals.end(),{0.f,0.f,1.f});
                    const float t=j==2?1.f:0.f;
                    mesh.colors.insert(mesh.colors.end(),{
                        std::clamp(p.rgb[0]*(1.f-t)+t,0.f,1.f),
                        std::clamp(p.rgb[1]*(1.f-t)+t*0.85f,0.f,1.f),
                        std::clamp(p.rgb[2]*(1.f-t)+t*0.22f,0.f,1.f),1.f});
                    mesh.uv0.insert(mesh.uv0.end(),{0.f,0.f,0.f,0.f});
                }
                mesh.indices.insert(mesh.indices.end(),{base,base+1,base+2});
            }
        }
    }
    scene.meshes.push_back(std::move(mesh));
    return scene;
}
} // namespace voxen::render::openbrush
