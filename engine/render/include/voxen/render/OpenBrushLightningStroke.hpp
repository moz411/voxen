#pragma once
#include <voxen/render/OpenBrushSmokeStroke.hpp>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace voxen::render::openbrush {
inline constexpr int LightningStrokePreset=33;
inline constexpr const char* LightningBrushGuid="voxen-native-lightning";
inline CompiledScene makeLightningStrokeScene(const std::vector<SmokeStrokePoint>& points) {
    CompiledScene scene; scene.formatVersion=2;
    if(points.empty())return scene;
    CompiledMesh mesh;mesh.brushGuid=LightningBrushGuid;
    for(std::size_t i=0;i<points.size();++i) {
        const auto& p=points[i];
        // Three branching forks, five ribbon segments each. Static buffers;
        // jitter and pulsing are evaluated by the vertex shader per eye.
        for(int branch=0;branch<3;++branch) {
            float seed=float(i*37+branch*19)*2.39996323f;
            float angle=seed+float(branch)*2.0943951f;
            float reach=p.size*(0.7f-0.16f*float(branch));
            float dx=std::cos(angle),dz=std::sin(angle);
            auto base=static_cast<std::uint32_t>(mesh.positions.size()/3);
            constexpr int steps=5;
            for(int row=0;row<=steps;++row) {
                float t=float(row)/float(steps);
                const std::array<float,3> center{
                    p.center[0]+dx*reach*t,
                    p.center[1]+reach*(0.30f*t+0.35f*std::sin(t*3.14f+seed)),
                    p.center[2]+dz*reach*t};
                for(int side=-1;side<=1;side+=2) {
                    mesh.positions.insert(mesh.positions.end(),center.begin(),center.end());
                    mesh.normals.insert(mesh.normals.end(),center.begin(),center.end());
                    mesh.colors.insert(mesh.colors.end(),{p.rgb[0],p.rgb[1],p.rgb[2],p.opacity});
                    mesh.uv0.insert(mesh.uv0.end(),{t,float(side),seed,p.size*0.014f});
                }
            }
            for(std::uint32_t row=0;row<steps;++row) {
                const auto a=base+row*2;
                mesh.indices.insert(mesh.indices.end(),{a,a+1,a+2,a+1,a+3,a+2});
            }
        }
    }
    scene.meshes.push_back(std::move(mesh));return scene;
}
} // namespace voxen::render::openbrush
