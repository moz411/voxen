#pragma once
#include <voxen/render/OpenBrushSmokeStroke.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

namespace voxen::render::openbrush {
inline constexpr int LightningStrokePreset=33;
inline constexpr const char* LightningBrushGuid="voxen-native-lightning";
namespace lightning_detail {
using Vec3=std::array<float,3>;
inline Vec3 point(const Vec3& origin,const Vec3& direction,float reach,float seed,float t) {
    // Stable, multi-scale kink: actual geometry stays fixed when reloaded.
    float kink=0.07f*reach*t*(std::sin(t*25.f+seed*7.f)+
        0.55f*std::sin(t*53.f-seed*11.f));
    return {origin[0]+direction[0]*reach*t+kink,
        origin[1]+direction[1]*reach*t,
        origin[2]+direction[2]*reach*t-kink*0.7f};
}
inline Vec3 direction(float angle,float rise) {
    float h=std::sqrt(std::max(0.f,1.f-rise*rise));
    return {std::cos(angle)*h,rise,std::sin(angle)*h};
}
} // namespace lightning_detail

inline CompiledScene makeLightningStrokeScene(const std::vector<SmokeStrokePoint>& points) {
    CompiledScene scene;scene.formatVersion=2;
    if(points.empty())return scene;
    CompiledMesh mesh;mesh.brushGuid=LightningBrushGuid;
    // Bound GPU memory/overdraw independently of long persisted strokes.
    const std::size_t stride=std::max<std::size_t>(1,(points.size()+1023)/1024);
    for(std::size_t i=0;i<points.size();i+=stride) {
        const auto& p=points[i];
        float seed=float(i)*2.39996323f;
        const auto trunk=lightning_detail::direction(seed,0.53f);
        const float baseReach=p.size*1.05f;
        auto emit=[&](const lightning_detail::Vec3& origin,
                      const lightning_detail::Vec3& direction,float reach,
                      float phase,int level) {
            const int steps=level==0?8:(level==1?5:3);
            const float width=p.size*(level==0?0.043f:(level==1?0.022f:0.011f));
            const float random=std::sin(phase*17.17f)*0.5f+0.5f;
            const float encoded=float(level)+std::min(random,0.999f);
            auto base=static_cast<std::uint32_t>(mesh.positions.size()/3);
            for(int row=0;row<=steps;++row) {
                float t=float(row)/float(steps);
                auto center=lightning_detail::point(origin,direction,reach,phase,t);
                auto before=lightning_detail::point(origin,direction,reach,phase,std::max(0.f,t-0.02f));
                auto after=lightning_detail::point(origin,direction,reach,phase,std::min(1.f,t+0.02f));
                float dx=after[0]-before[0],dy=after[1]-before[1],dz=after[2]-before[2];
                float len=std::max(1e-6f,std::sqrt(dx*dx+dy*dy+dz*dz));
                for(int side=-1;side<=1;side+=2) {
                    // inPosition = center + tangent; inCenter = actual arc point.
                    mesh.positions.insert(mesh.positions.end(),{
                        center[0]+dx/len,center[1]+dy/len,center[2]+dz/len});
                    mesh.normals.insert(mesh.normals.end(),center.begin(),center.end());
                    mesh.colors.insert(mesh.colors.end(),{p.rgb[0],p.rgb[1],p.rgb[2],p.opacity});
                    mesh.uv0.insert(mesh.uv0.end(),{t,float(side),encoded,width});
                }
            }
            for(int row=0;row<steps;++row) {
                auto a=base+static_cast<std::uint32_t>(row*2);
                mesh.indices.insert(mesh.indices.end(),{a,a+1,a+2,a+1,a+3,a+2});
            }
        };
        emit(p.center,trunk,baseReach,seed,0);
        // Two children attach exactly to the trunk. Each spawns two micro-arcs.
        for(int branch=0;branch<2;++branch) {
            float t=0.36f+float(branch)*0.31f;
            auto origin=lightning_detail::point(p.center,trunk,baseReach,seed,t);
            float phase=seed+float(branch+1)*3.17f;
            auto direction=lightning_detail::direction(phase+1.8f,0.21f+0.12f*float(branch));
            float reach=baseReach*(0.46f-0.08f*float(branch));
            emit(origin,direction,reach,phase,1);
            for(int micro=0;micro<2;++micro) {
                float attach=0.44f+0.30f*float(micro);
                auto start=lightning_detail::point(origin,direction,reach,phase,attach);
                float microPhase=phase+float(micro+1)*4.13f;
                emit(start,lightning_detail::direction(microPhase+2.4f,-0.12f),
                    reach*(0.42f-0.12f*float(micro)),microPhase,2);
            }
        }
    }
    scene.meshes.push_back(std::move(mesh));return scene;
}
} // namespace voxen::render::openbrush
