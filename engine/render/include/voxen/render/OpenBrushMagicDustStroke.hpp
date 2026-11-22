#pragma once
#include <voxen/render/OpenBrushSmokeStroke.hpp>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace voxen::render::openbrush {
inline constexpr int MagicDustStrokePreset=35;
inline constexpr const char* MagicDustBrushGuid="voxen-native-magic-dust";
inline CompiledScene makeMagicDustStrokeScene(const std::vector<SmokeStrokePoint>& points) {
    CompiledScene scene;scene.formatVersion=2;
    if(points.empty())return scene;
    CompiledMesh mesh;mesh.brushGuid=MagicDustBrushGuid;
    constexpr float corners[4][2]={{-1.f,-1.f},{1.f,-1.f},{1.f,1.f},{-1.f,1.f}};
    for(std::size_t i=0;i<points.size();++i) {
        const auto& p=points[i];
        for(int n=0;n<7;++n) {
            float phase=float(i*41+n*23)*2.39996323f;
            float spread=p.size*0.62f;
            const std::array<float,3> center{p.center[0]+std::sin(phase)*spread,
                p.center[1]+std::cos(phase*1.31f)*spread,
                p.center[2]+std::sin(phase*0.71f)*spread};
            const float radius=p.size*(0.018f+0.007f*float(n%3));
            auto base=static_cast<std::uint32_t>(mesh.positions.size()/3);
            for(const auto& c:corners) {
                mesh.positions.insert(mesh.positions.end(),center.begin(),center.end());
                mesh.normals.insert(mesh.normals.end(),center.begin(),center.end());
                mesh.colors.insert(mesh.colors.end(),{p.rgb[0],p.rgb[1],p.rgb[2],p.opacity});
                mesh.uv0.insert(mesh.uv0.end(),{c[0],c[1],phase,radius});
            }
            mesh.indices.insert(mesh.indices.end(),{base,base+1,base+2,base,base+2,base+3});
        }
    }
    scene.meshes.push_back(std::move(mesh));return scene;
}
} // namespace voxen::render::openbrush
