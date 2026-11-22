#pragma once
// Multi-segment animated flame ribbons with a tapered, curved silhouette.
#include <voxen/render/OpenBrushSmokeStroke.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>
namespace voxen::render::openbrush {
inline constexpr int FireStrokePreset=29;
inline CompiledScene makeFireStrokeScene(const std::vector<SmokeStrokePoint>& points, float timeSeconds=0.f) {
    (void)timeSeconds;
    CompiledScene scene;
    scene.formatVersion=2;
    if(points.empty())return scene;
    CompiledMesh mesh;
    mesh.brushGuid="voxen-native-fire";
    constexpr int steps=6;
    for(std::size_t i=0;i<points.size();++i) {
        const auto& p=points[i];
        for(int tongue=0;tongue<5;++tongue) {
            const float phase=float(i*13+tongue*31)*2.39996323f;
            const float radius=p.size*(0.075f+0.017f*float(tongue));
            const float height=p.size*(0.45f+0.13f*float(tongue));
            const float cx=p.center[0]+std::sin(phase)*p.size*0.16f;
            const float cz=p.center[2]+std::cos(phase)*p.size*0.16f;
            for(int plane=0;plane<2;++plane) {
                const float axis=phase+float(plane)*1.57079632679f;
                const float dx=std::cos(axis)*radius;
                const float dz=std::sin(axis)*radius;
                const auto base=static_cast<std::uint32_t>(mesh.positions.size()/3);
                for(int row=0;row<=steps;++row) {
                    const float t=float(row)/float(steps);
                    const float width=std::pow(1.f-t,1.35f);
                    const float curve=std::sin(t*2.4f+phase)*radius*t*t*0.65f;
                    for(int side=-1;side<=1;side+=2) {
                        mesh.positions.insert(mesh.positions.end(),{
                            cx+dx*width*float(side)+curve,p.center[1]+height*t,
                            cz+dz*width*float(side)+curve*0.5f});
                        mesh.normals.insert(mesh.normals.end(),{cx,p.center[1],cz});
                        mesh.colors.insert(mesh.colors.end(),{
                            std::clamp(1.f-0.12f*t,0.f,1.f),
                            std::clamp(0.28f+0.70f*t,0.f,1.f),
                            std::clamp(0.025f+0.28f*t,0.f,1.f),
                            (0.8f-0.55f*t)*std::clamp(p.opacity,0.f,1.f)});
                        mesh.uv0.insert(mesh.uv0.end(),{t,phase,radius,height});
                    }
                }
                for(std::uint32_t row=0;row<steps;++row) {
                    const auto v=base+row*2;
                    mesh.indices.insert(mesh.indices.end(),{v,v+1,v+2,v+1,v+3,v+2});
                }
            }
        }
    }
    scene.meshes.push_back(std::move(mesh));
    return scene;
}
} // namespace voxen::render::openbrush
