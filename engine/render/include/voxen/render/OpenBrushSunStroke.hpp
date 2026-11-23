#pragma once
// Sun: a glowing camera-facing stellar disk with procedural corona.
// Four vertices per sample; all turbulence and glow are computed in GLSL.
#include <voxen/render/OpenBrushSmokeStroke.hpp>
namespace voxen::render::openbrush {
inline constexpr int SunStrokePreset=38; // Preserve existing native IDs 27..37.
inline CompiledScene makeSunStrokeScene(const std::vector<SmokeStrokePoint>& points) {
    CompiledScene scene;scene.formatVersion=2;
    if(points.empty())return scene;
    CompiledMesh mesh;mesh.brushGuid="voxen-native-sun";
    constexpr float corners[4][2]={{-1.f,-1.f},{1.f,-1.f},{1.f,1.f},{-1.f,1.f}};
    for(std::size_t i=0;i<points.size();++i) {
        const auto& p=points[i];
        const float radius=p.size*1.65f; // disk + broad procedural halo
        const auto base=static_cast<std::uint32_t>(mesh.positions.size()/3);
        for(const auto& corner:corners) {
            mesh.positions.insert(mesh.positions.end(),p.center.begin(),p.center.end());
            mesh.normals.insert(mesh.normals.end(),p.center.begin(),p.center.end());
            mesh.colors.insert(mesh.colors.end(),{p.rgb[0],p.rgb[1],p.rgb[2],p.opacity});
            mesh.uv0.insert(mesh.uv0.end(),{corner[0],corner[1],float(i)*2.39996323f,radius});
        }
        mesh.indices.insert(mesh.indices.end(),{base,base+1,base+2,base,base+2,base+3});
    }
    scene.meshes.push_back(std::move(mesh));
    return scene;
}
}
