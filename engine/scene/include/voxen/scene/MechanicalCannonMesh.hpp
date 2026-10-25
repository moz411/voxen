#pragma once
#include <voxen/scene/MechanicalCannon.hpp>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

namespace voxen::scene::cannon {
struct Vertex { Vec3 position; Vec3 normal; };
struct Mesh { std::vector<Vertex> vertices; std::vector<std::uint32_t> indices; };
inline void quad(Mesh& m, Vec3 a, Vec3 b, Vec3 c, Vec3 d, Vec3 n) {
    const auto start=static_cast<std::uint32_t>(m.vertices.size());
    m.vertices.insert(m.vertices.end(),{{a,n},{b,n},{c,n},{d,n}});
    m.indices.insert(m.indices.end(),{start,start+1,start+2,start,start+2,start+3});
}
inline Mesh boxMesh(Vec3 h) {
    Mesh m;
    quad(m,{-h.x,-h.y,h.z},{h.x,-h.y,h.z},{h.x,h.y,h.z},{-h.x,h.y,h.z},{0,0,1});
    quad(m,{h.x,-h.y,-h.z},{-h.x,-h.y,-h.z},{-h.x,h.y,-h.z},{h.x,h.y,-h.z},{0,0,-1});
    quad(m,{h.x,-h.y,h.z},{h.x,-h.y,-h.z},{h.x,h.y,-h.z},{h.x,h.y,h.z},{1,0,0});
    quad(m,{-h.x,-h.y,-h.z},{-h.x,-h.y,h.z},{-h.x,h.y,h.z},{-h.x,h.y,-h.z},{-1,0,0});
    quad(m,{-h.x,h.y,h.z},{h.x,h.y,h.z},{h.x,h.y,-h.z},{-h.x,h.y,-h.z},{0,1,0});
    quad(m,{-h.x,-h.y,-h.z},{h.x,-h.y,-h.z},{h.x,-h.y,h.z},{-h.x,-h.y,h.z},{0,-1,0});
    return m;
}
// Cylinder along local +Z; hollow=false is suitable for pistons and cooling-ring proxies.
// Meshes are authored around their center and transformed by the renderer.
inline Mesh cylinderMesh(float radius,float halfLength,unsigned segments=24) {
    Mesh m;
    if(!(radius>0) || !(halfLength>0) || segments<3 || segments>128)return m;
    constexpr float tau=6.2831853071795864769f;
    for(unsigned i=0;i<segments;++i) {
        float a=tau*i/segments,b=tau*(i+1)/segments;
        Vec3 n0{std::cos(a),std::sin(a),0},n1{std::cos(b),std::sin(b),0};
        Vec3 a0=n0*radius+Vec3{0,0,-halfLength},a1=n1*radius+Vec3{0,0,-halfLength};
        Vec3 b0=n0*radius+Vec3{0,0,halfLength},b1=n1*radius+Vec3{0,0,halfLength};
        const auto k=static_cast<std::uint32_t>(m.vertices.size());
        m.vertices.insert(m.vertices.end(),{{a0,n0},{a1,n1},{b1,n1},{b0,n0}});
        m.indices.insert(m.indices.end(),{k,k+1,k+2,k,k+2,k+3});
        quad(m,{0,0,halfLength},b0,b1,{0,0,halfLength},{0,0,1});
        quad(m,{0,0,-halfLength},a1,a0,{0,0,-halfLength},{0,0,-1});
    }
    return m;
}
inline Mesh partMesh(const MechanicalPart& part) {
    switch(part.kind) {
        case Part::Barrel: case Part::PistonLeft: case Part::PistonRight:
        case Part::CoolingRing: case Part::Muzzle: case Part::ChargingCore:
            return cylinderMesh(part.halfExtents.x,part.halfExtents.z);
        default: return boxMesh(part.halfExtents);
    }
}
} // namespace voxen::scene::cannon
