#pragma once
// Camera-facing translucent bubble disks with iridescent shader rims.
#include <voxen/render/OpenBrushSmokeStroke.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>
namespace voxen::render::openbrush {
inline constexpr int BubblesStrokePreset=32;
inline CompiledScene makeBubblesStrokeScene(const std::vector<SmokeStrokePoint>& points) {
    CompiledScene scene;
    scene.formatVersion=2;
    if(points.empty())return scene;
    CompiledMesh mesh;
    mesh.brushGuid="voxen-native-bubbles";
    // Four camera-facing corners; the fragment shader draws the circular rim.
    // This is 2 triangles instead of a 32-triangle fan per bubble.
    for(std::size_t i=0;i<points.size();++i) {
        const auto& p=points[i];
        for(int n=0;n<3;++n) {
            const float phase=float(i*37+n*23)*2.39996323f;
            const float spread=p.size*0.4f;
            const std::array<float,3> center{
                p.center[0]+std::sin(phase)*spread,
                p.center[1]+std::cos(phase*1.4f)*spread,
                p.center[2]+std::sin(phase*0.6f)*spread};
            const float radius=p.size*(0.19f+0.045f*float(n));
            const std::uint32_t base=static_cast<std::uint32_t>(mesh.positions.size()/3);
            constexpr float corners[4][2]={{-1.f,-1.f},{1.f,-1.f},{1.f,1.f},{-1.f,1.f}};
            for(const auto& corner:corners) {
                const float x=corner[0],y=corner[1];
                mesh.positions.insert(mesh.positions.end(),{
                    center[0]+x*radius,center[1]+y*radius,center[2]});
                mesh.normals.insert(mesh.normals.end(),center.begin(),center.end());
                mesh.colors.insert(mesh.colors.end(),{
                    std::clamp(p.rgb[0],0.f,1.f),std::clamp(p.rgb[1],0.f,1.f),
                    std::clamp(p.rgb[2],0.f,1.f),0.95f});
                mesh.uv0.insert(mesh.uv0.end(),{x,y,phase,radius});
            }
            mesh.indices.insert(mesh.indices.end(),{base,base+1,base+2,base,base+2,base+3});
        }
    }
    scene.meshes.push_back(std::move(mesh));
    return scene;
}
} // namespace voxen::render::openbrush
