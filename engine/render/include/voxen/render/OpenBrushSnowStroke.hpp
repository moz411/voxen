#pragma once
// Deterministic native Snow particles. The GPU animates positions.
#include <voxen/render/OpenBrushSmokeStroke.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>
namespace voxen::render::openbrush {
inline constexpr int SnowStrokePreset=30;
inline CompiledScene makeSnowStrokeScene(const std::vector<SmokeStrokePoint>& points) {
    CompiledScene scene;
    scene.formatVersion=2;
    if(points.empty())return scene;
    CompiledMesh mesh;
    mesh.brushGuid="voxen-native-snow";
    constexpr float tau=6.28318530718f;
    for(std::size_t i=0;i<points.size();++i) {
        const auto& p=points[i];
        for(int n=0;n<5;++n) {
            const float phase=float(i*37+n*17)*2.39996323f;
            const float spread=p.size*0.65f;
            const std::array<float,3> center{
                p.center[0]+std::sin(phase)*spread,
                p.center[1]+std::cos(phase*1.31f)*spread,
                p.center[2]+std::sin(phase*0.77f)*spread};
            const float radius=p.size*(0.022f+0.007f*float(n%3));
            // Crossed six-spoke snowflake, visible from different angles.
            for(int plane=0;plane<2;++plane) {
                const std::uint32_t base=static_cast<std::uint32_t>(mesh.positions.size()/3);
                for(int j=0;j<12;++j) {
                    const float a=float(j)*tau/12.f;
                    const float r=(j%2==0)?radius:radius*0.32f;
                    const float x=std::cos(a)*r,y=std::sin(a)*r;
                    mesh.positions.insert(mesh.positions.end(),{
                        center[0]+(plane?0.f:x),center[1]+y,center[2]+(plane?x:0.f)});
                    mesh.normals.insert(mesh.normals.end(),center.begin(),center.end());
                    mesh.colors.insert(mesh.colors.end(),{
                        std::clamp(p.rgb[0],0.f,1.f),std::clamp(p.rgb[1],0.f,1.f),
                        std::clamp(p.rgb[2],0.f,1.f),0.85f});
                    mesh.uv0.insert(mesh.uv0.end(),{phase,float(j),radius,0.f});
                }
                for(std::uint32_t j=1;j<11;++j)
                    mesh.indices.insert(mesh.indices.end(),{base,base+j,base+j+1});
            }
        }
    }
    scene.meshes.push_back(std::move(mesh));
    return scene;
}
} // namespace voxen::render::openbrush
