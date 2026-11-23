#pragma once
// Deterministic chaotic architecture: interlocking random cuboids per stroke sample.
// Compact indexed geometry, flat-shaded faces, no runtime allocations per frame.
#include <voxen/render/OpenBrushSmokeStroke.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
namespace voxen::render::openbrush {
inline constexpr int ChaosCityStrokePreset=39;
inline std::uint32_t chaosHash(std::uint32_t x) {
    x^=x>>16;x*=0x7feb352du;x^=x>>15;x*=0x846ca68bu;x^=x>>16;return x;
}
inline float chaosRandom(std::uint32_t seed) {
    return float(chaosHash(seed)&0x00ffffffu)/16777216.f;
}
inline CompiledScene makeChaosCityStrokeScene(const std::vector<SmokeStrokePoint>& points) {
    CompiledScene scene;scene.formatVersion=2;
    if(points.empty())return scene;
    CompiledMesh mesh;mesh.brushGuid="voxen-native-chaos-city";
    constexpr int faces[6][4]={{0,1,2,3},{5,4,7,6},{4,0,3,7},
        {1,5,6,2},{3,2,6,7},{4,5,1,0}};
    constexpr float normals[6][3]={{0,0,-1},{0,0,1},{-1,0,0},
        {1,0,0},{0,1,0},{0,-1,0}};
    for(std::size_t i=0;i<points.size();++i) {
        const auto& p=points[i];
        // 7 blocks per point; overlap and varying height create stacked megastructures.
        for(int n=0;n<7;++n) {
            const std::uint32_t seed=std::uint32_t(i)*0x9e3779b9u+std::uint32_t(n)*0x85ebca6bu;
            const float size=p.size;
            const float sx=size*(.08f+.28f*chaosRandom(seed+1));
            const float sy=size*(.14f+1.25f*chaosRandom(seed+2));
            const float sz=size*(.08f+.28f*chaosRandom(seed+3));
            const float cx=p.center[0]+size*(chaosRandom(seed+4)-.5f)*.85f;
            const float cy=p.center[1]+size*(chaosRandom(seed+5)-.3f)*.55f;
            const float cz=p.center[2]+size*(chaosRandom(seed+6)-.5f)*.85f;
            const float baseColor=.25f+.75f*chaosRandom(seed+7);
            const std::array<float,3> color{
                std::clamp(p.rgb[0]*.65f+baseColor*.35f,0.f,1.f),
                std::clamp(p.rgb[1]*.65f+chaosRandom(seed+8)*.35f,0.f,1.f),
                std::clamp(p.rgb[2]*.65f+chaosRandom(seed+9)*.35f,0.f,1.f)};
            const std::array<std::array<float,3>,8> corners{{
                {{cx-sx,cy-sy,cz-sz}},{{cx+sx,cy-sy,cz-sz}},
                {{cx+sx,cy+sy,cz-sz}},{{cx-sx,cy+sy,cz-sz}},
                {{cx-sx,cy-sy,cz+sz}},{{cx+sx,cy-sy,cz+sz}},
                {{cx+sx,cy+sy,cz+sz}},{{cx-sx,cy+sy,cz+sz}}
            }};
            for(int f=0;f<6;++f) {
                const auto base=static_cast<std::uint32_t>(mesh.positions.size()/3);
                for(int j=0;j<4;++j) {
                    const auto& xyz=corners[faces[f][j]];
                    mesh.positions.insert(mesh.positions.end(),xyz.begin(),xyz.end());
                    mesh.normals.insert(mesh.normals.end(),normals[f],normals[f]+3);
                    mesh.colors.insert(mesh.colors.end(),{color[0],color[1],color[2],p.opacity});
                    mesh.uv0.insert(mesh.uv0.end(),{float(i)*.13f,float(n)*.37f,0.f,0.f});
                }
                mesh.indices.insert(mesh.indices.end(),{base,base+1,base+2,base,base+2,base+3});
            }
        }
    }
    scene.meshes.push_back(std::move(mesh));
    return scene;
}
}
