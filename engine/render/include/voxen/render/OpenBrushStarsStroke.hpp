#pragma once
// Deterministic native Stars brush, shared OpenBrush compiled-mesh format.
#include <voxen/render/OpenBrushSmokeStroke.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>
namespace voxen::render::openbrush {
inline constexpr int StarsStrokePreset=28;
inline CompiledScene makeStarsStrokeScene(const std::vector<SmokeStrokePoint>& points) {
    CompiledScene scene;
    scene.formatVersion=2;
    if(points.empty())return scene;
    CompiledMesh mesh;
    mesh.brushGuid="voxen-native-stars";
    constexpr float pi=3.14159265358979323846f;
    for(std::size_t i=0;i<points.size();++i) {
        const auto& p=points[i];
        for(int n=0;n<3;++n) {
            const float angle=float(i*17+n*43)*2.39996323f;
            const float spread=p.size*0.38f;
            const std::array<float,3> center{
                p.center[0]+std::cos(angle)*spread,
                p.center[1]+std::sin(angle)*spread,
                p.center[2]+std::sin(angle*1.7f)*spread*0.6f};
            const float radius=p.size*(0.035f+0.015f*float(n));
            for(int plane=0;plane<2;++plane) {
                const std::uint32_t base=static_cast<std::uint32_t>(mesh.positions.size()/3);
                const float tilt=plane?pi*0.25f:0.f;
                for(int j=0;j<8;++j) {
                    const float a=tilt+float(j)*pi*0.25f;
                    const float r=(j%2==0)?radius:radius*0.22f;
                    mesh.positions.insert(mesh.positions.end(),{
                        center[0]+std::cos(a)*r,center[1]+std::sin(a)*r,center[2]});
                    mesh.normals.insert(mesh.normals.end(),{0.f,0.f,1.f});
                    mesh.colors.insert(mesh.colors.end(),{
                        std::clamp(p.rgb[0],0.f,1.f),std::clamp(p.rgb[1],0.f,1.f),
                        std::clamp(p.rgb[2],0.f,1.f),1.f});
                    mesh.uv0.insert(mesh.uv0.end(),{0.f,0.f,0.f,0.f});
                }
                for(std::uint32_t j=1;j<7;++j)
                    mesh.indices.insert(mesh.indices.end(),{base,base+j,base+j+1});
            }
        }
    }
    scene.meshes.push_back(std::move(mesh));
    return scene;
}
} // namespace voxen::render::openbrush
