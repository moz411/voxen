#pragma once
// True 3D solar sphere with animated emissive surface, plus a separate
// camera-facing corona quad. Shared brush shader selects the geometry mode.
#include <voxen/render/OpenBrushSmokeStroke.hpp>
#include <algorithm>
#include <cmath>
#include <cstdint>
namespace voxen::render::openbrush {
inline constexpr int SunStrokePreset=38;
inline CompiledScene makeSunStrokeScene(const std::vector<SmokeStrokePoint>& points) {
    CompiledScene scene;scene.formatVersion=2;
    if(points.empty())return scene;
    CompiledMesh mesh;mesh.brushGuid="voxen-native-sun";
    constexpr int lat=12,lon=20;
    constexpr float pi=3.14159265358979323846f;
    for(std::size_t i=0;i<points.size();++i) {
        const auto& p=points[i];
        const float radius=p.size*.62f;
        const auto base=static_cast<std::uint32_t>(mesh.positions.size()/3);
        for(int y=0;y<=lat;++y) {
            const float a=pi*float(y)/float(lat);
            for(int x=0;x<=lon;++x) {
                const float b=2.f*pi*float(x)/float(lon);
                const float nx=std::sin(a)*std::cos(b);
                const float ny=std::cos(a);
                const float nz=std::sin(a)*std::sin(b);
                mesh.positions.insert(mesh.positions.end(),{
                    p.center[0]+radius*nx,p.center[1]+radius*ny,p.center[2]+radius*nz});
                mesh.normals.insert(mesh.normals.end(),{nx,ny,nz});
                mesh.colors.insert(mesh.colors.end(),{p.rgb[0],p.rgb[1],p.rgb[2],p.opacity});
                mesh.uv0.insert(mesh.uv0.end(),{nx,ny,-1.f,nz});
            }
        }
        for(int y=0;y<lat;++y)for(int x=0;x<lon;++x) {
            const auto a=base+static_cast<std::uint32_t>(y*(lon+1)+x);
            const auto b=a+static_cast<std::uint32_t>(lon+1);
            mesh.indices.insert(mesh.indices.end(),{a,b,a+1,a+1,b,b+1});
        }
        // Halo billboard surrounds the solid sphere without replacing it.
        constexpr float corners[4][2]={{-1.f,-1.f},{1.f,-1.f},{1.f,1.f},{-1.f,1.f}};
        const auto quad=static_cast<std::uint32_t>(mesh.positions.size()/3);
        for(const auto& corner:corners) {
            mesh.positions.insert(mesh.positions.end(),p.center.begin(),p.center.end());
            mesh.normals.insert(mesh.normals.end(),{0.f,0.f,1.f});
            mesh.colors.insert(mesh.colors.end(),{p.rgb[0],p.rgb[1],p.rgb[2],p.opacity});
            mesh.uv0.insert(mesh.uv0.end(),{corner[0],corner[1],float(i)*2.39996323f,p.size*1.65f});
        }
        mesh.indices.insert(mesh.indices.end(),{quad,quad+1,quad+2,quad,quad+2,quad+3});
    }
    scene.meshes.push_back(std::move(mesh));
    return scene;
}
}
