#pragma once
#include <voxen/render/OpenBrushSmokeStroke.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>
namespace voxen::render::openbrush {
inline constexpr int StarsStrokePreset=28;
inline CompiledScene makeStarsStrokeScene(const std::vector<SmokeStrokePoint>& points) {
    CompiledScene scene; scene.formatVersion=2;
    if(points.empty())return scene;
    CompiledMesh mesh; mesh.brushGuid="voxen-native-stars";
    constexpr float tau=6.283185307179586f;
    constexpr int segments=12;
    for(std::size_t i=0;i<points.size();++i) {
        const auto& p=points[i];
        for(int n=0;n<6;++n) {
            const float phase=float(i*17+n*43)*2.39996323f;
            const float spread=p.size*0.55f;
            const std::array<float,3> center{
                p.center[0]+std::cos(phase)*spread,
                p.center[1]+std::sin(phase*1.3f)*spread,
                p.center[2]+std::sin(phase*1.7f)*spread*0.6f};
            const float radius=p.size*(0.075f+0.016f*float(n%3));
            const auto base=static_cast<std::uint32_t>(mesh.positions.size()/3);
            for(int j=0;j<=segments;++j) {
                const float angle=float(j==0?0:j-1)*tau/float(segments);
                const float x=j==0?0.f:std::cos(angle);
                const float y=j==0?0.f:std::sin(angle);
                mesh.positions.insert(mesh.positions.end(),center.begin(),center.end());
                mesh.normals.insert(mesh.normals.end(),center.begin(),center.end());
                mesh.colors.insert(mesh.colors.end(),{
                    std::clamp(p.rgb[0]+0.3f,0.f,1.f),
                    std::clamp(p.rgb[1]+0.3f,0.f,1.f),
                    std::clamp(p.rgb[2]+0.4f,0.f,1.f),0.9f});
                mesh.uv0.insert(mesh.uv0.end(),{x,y,-(phase+1.f),radius});
            }
            for(std::uint32_t j=1;j<=segments;++j)
                mesh.indices.insert(mesh.indices.end(),{base,base+j,base+1+(j%segments)});
        }
    }
    scene.meshes.push_back(std::move(mesh));
    return scene;
}
}
