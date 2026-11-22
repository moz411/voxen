#pragma once
#include <voxen/render/OpenBrushSmokeStroke.hpp>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace voxen::render::openbrush {
inline constexpr int NeonTrailsStrokePreset=34;
inline constexpr const char* NeonTrailsBrushGuid="voxen-native-neon-trails";
inline CompiledScene makeNeonTrailsStrokeScene(const std::vector<SmokeStrokePoint>& points) {
    CompiledScene scene;scene.formatVersion=2;
    if(points.size()<2)return scene;
    CompiledMesh mesh;mesh.brushGuid=NeonTrailsBrushGuid;
    for(std::size_t i=1;i<points.size();++i) {
        const auto& a=points[i-1];const auto& b=points[i];
        const float dx=b.center[0]-a.center[0],dy=b.center[1]-a.center[1],dz=b.center[2]-a.center[2];
        const float distance2=dx*dx+dy*dy+dz*dz;
        const float maxGap=std::max(a.size,b.size)*0.6f;
        // Never bridge unrelated trigger strokes or teleportation gaps.
        if(distance2<1e-8f || distance2>maxGap*maxGap)continue;
        auto base=static_cast<std::uint32_t>(mesh.positions.size()/3);
        const float width=std::min(a.size,b.size)*0.045f;
        for(int end=0;end<2;++end) {
            const auto& p=end?b:a;
            const auto& other=end?a:b;
            for(int side=-1;side<=1;side+=2) {
                mesh.positions.insert(mesh.positions.end(),other.center.begin(),other.center.end());
                mesh.normals.insert(mesh.normals.end(),p.center.begin(),p.center.end());
                mesh.colors.insert(mesh.colors.end(),{p.rgb[0],p.rgb[1],p.rgb[2],p.opacity});
                mesh.uv0.insert(mesh.uv0.end(),{float(side),float(end),float(i)*0.173f,width});
            }
        }
        mesh.indices.insert(mesh.indices.end(),{base,base+1,base+2,base+1,base+3,base+2});
    }
    if(!mesh.indices.empty())scene.meshes.push_back(std::move(mesh));
    return scene;
}
} // namespace voxen::render::openbrush
