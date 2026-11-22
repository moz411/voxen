#pragma once
#include <voxen/render/OpenBrushSmokeStroke.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

namespace voxen::render::openbrush {
inline constexpr int NeonTrailsStrokePreset=34;
inline constexpr const char* NeonTrailsBrushGuid="voxen-native-neon-trails";
namespace neon_detail {
using Vec3=std::array<float,3>;
inline Vec3 delta(const Vec3& a,const Vec3& b) {
    return {a[0]-b[0],a[1]-b[1],a[2]-b[2]};
}
inline float distance(const Vec3& a,const Vec3& b) {
    auto v=delta(a,b);return std::sqrt(v[0]*v[0]+v[1]*v[1]+v[2]*v[2]);
}
inline Vec3 blend(const Vec3& a,const Vec3& b,float alpha) {
    return {a[0]*(1.f-alpha)+b[0]*alpha,a[1]*(1.f-alpha)+b[1]*alpha,
        a[2]*(1.f-alpha)+b[2]*alpha};
}
inline Vec3 catmull(const Vec3& a,const Vec3& b,const Vec3& c,const Vec3& d,float t) {
    // Centripetal Catmull-Rom for nonuniform hand-controller samples.
    float k0=0.f,k1=std::sqrt(std::max(1e-6f,distance(a,b)));
    float k2=k1+std::sqrt(std::max(1e-6f,distance(b,c)));
    float k3=k2+std::sqrt(std::max(1e-6f,distance(c,d)));
    float u=k1+(k2-k1)*t;
    auto lerp=[](const Vec3& x,const Vec3& y,float x0,float x1,float v) {
        return blend(x,y,(v-x0)/std::max(1e-6f,x1-x0));
    };
    Vec3 aa=lerp(a,b,k0,k1,u),bb=lerp(b,c,k1,k2,u),cc=lerp(c,d,k2,k3,u);
    return lerp(lerp(aa,bb,k0,k2,u),lerp(bb,cc,k1,k3,u),k1,k2,u);
}
} // namespace neon_detail

inline CompiledScene makeNeonTrailsStrokeScene(const std::vector<SmokeStrokePoint>& points) {
    CompiledScene scene;scene.formatVersion=2;
    if(points.size()<2)return scene;
    CompiledMesh mesh;mesh.brushGuid=NeonTrailsBrushGuid;
    std::size_t start=0;
    while(start+1<points.size()) {
        std::size_t end=start+1;
        while(end<points.size()) {
            float gap=neon_detail::distance(points[end-1].center,points[end].center);
            if(gap<1e-5f || gap>std::max(points[end-1].size,points[end].size)*0.6f)break;
            ++end;
        }
        if(end>start+1) {
            // Distance-based stroke points are already dense. Lower tessellation
            // for long strokes to keep headset CPU rebuilds and uploads bounded.
            const int subdivisions=end-start>1024?2:(end-start>512?3:4);
            const std::size_t count=(end-start-1)*subdivisions;
            const auto base=static_cast<std::uint32_t>(mesh.positions.size()/3);
            std::vector<neon_detail::Vec3> centers;
            centers.reserve(count+1);
            for(std::size_t k=0;k<=count;++k) {
                std::size_t seg=std::min(k/std::size_t(subdivisions),end-start-2);
                float t=k==count?1.f:float(k%subdivisions)/float(subdivisions);
                std::size_t j=start+seg;
                centers.push_back(neon_detail::catmull(
                    points[j>start?j-1:j].center,points[j].center,points[j+1].center,
                    points[j+2<end?j+2:j+1].center,t));
            }
            float distanceAlong=0.f;
            for(std::size_t k=0;k<centers.size();++k) {
                if(k)distanceAlong+=neon_detail::distance(centers[k-1],centers[k]);
                auto tangent=neon_detail::delta(centers[std::min(k+1,count)],centers[k?k-1:k]);
                float length=std::max(1e-6f,std::sqrt(tangent[0]*tangent[0]+
                    tangent[1]*tangent[1]+tangent[2]*tangent[2]));
                const auto& p=points[std::min(start+k/subdivisions,end-1)];
                float fade=std::min(1.f,float(std::min(k,count-k))*0.4f);
                float width=p.size*0.12f*(0.55f+0.45f*fade);
                for(int side=-1;side<=1;side+=2) {
                    // inPosition = center + tangent; inCenter = curve position.
                    mesh.positions.insert(mesh.positions.end(),{
                        centers[k][0]+tangent[0]/length,
                        centers[k][1]+tangent[1]/length,
                        centers[k][2]+tangent[2]/length});
                    mesh.normals.insert(mesh.normals.end(),centers[k].begin(),centers[k].end());
                    mesh.colors.insert(mesh.colors.end(),{p.rgb[0],p.rgb[1],p.rgb[2],p.opacity});
                    mesh.uv0.insert(mesh.uv0.end(),{float(side),distanceAlong,float(start)*0.73f,width});
                }
                if(k) {
                    auto a=base+static_cast<std::uint32_t>((k-1)*2);
                    mesh.indices.insert(mesh.indices.end(),{a,a+1,a+2,a+1,a+3,a+2});
                }
            }
        }
        // Continue from the endpoint after a gap, without bridging strokes.
        start=end;
    }
    if(!mesh.indices.empty())scene.meshes.push_back(std::move(mesh));
    return scene;
}
} // namespace voxen::render::openbrush
