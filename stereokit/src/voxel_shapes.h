#pragma once
#include <stereokit.h>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

namespace voxen_shapes {
using namespace sk;
constexpr int count = 8;
inline constexpr const char* labels[count] = {
    "CUBE", "SPHERE", "TETRA", "OCTAHEDRON",
    "CROSS", "STAR", "CYLINDER", "DIAMOND"
};

struct Builder {
    std::vector<vert_t> vertices;
    std::vector<uint32_t> indices;
    void triangle(vec3 a, vec3 b, vec3 c) {
        const vec3 u{b.x-a.x,b.y-a.y,b.z-a.z};
        const vec3 v{c.x-a.x,c.y-a.y,c.z-a.z};
        vec3 n{u.y*v.z-u.z*v.y,u.z*v.x-u.x*v.z,u.x*v.y-u.y*v.x};
        const float len=std::sqrt(n.x*n.x+n.y*n.y+n.z*n.z);
        if(len<1e-8f) return;
        n={n.x/len,n.y/len,n.z/len};
        const uint32_t start=static_cast<uint32_t>(vertices.size());
        for(vec3 p:{a,b,c}) {
            vert_t vertex{};
            vertex.pos=p; vertex.norm=n;
            vertex.uv={p.x+0.5f,p.y+0.5f};
            vertex.col={255,255,255,255};
            vertices.push_back(vertex);
        }
        indices.insert(indices.end(),{start,start+1,start+2});
    }
    mesh_t finish() {
        mesh_t mesh=mesh_create();
        mesh_set_verts(mesh,vertices.data(),static_cast<int32_t>(vertices.size()),true);
        mesh_set_inds(mesh,indices.data(),static_cast<int32_t>(indices.size()));
        return mesh;
    }
};

inline mesh_t prism(const std::vector<vec2>& outline,float depth) {
    Builder b;
    const float z=depth*0.5f;
    const vec3 front{0,0,z},back{0,0,-z};
    for(size_t i=0;i<outline.size();++i) {
        const vec2 p=outline[i],q=outline[(i+1)%outline.size()];
        const vec3 a{p.x,p.y,z},c{q.x,q.y,z};
        const vec3 d{p.x,p.y,-z},e{q.x,q.y,-z};
        b.triangle(front,a,c);
        b.triangle(back,e,d);
        b.triangle(a,d,e);
        b.triangle(a,e,c);
    }
    return b.finish();
}

inline mesh_t bipyramid(int sides,float radius,float halfheight) {
    Builder b;
    constexpr float pi=3.14159265358979323846f;
    const vec3 top{0,halfheight,0},bottom{0,-halfheight,0};
    for(int i=0;i<sides;++i) {
        const float a=2*pi*i/sides,beta=2*pi*(i+1)/sides;
        const vec3 p{radius*std::cos(a),0,radius*std::sin(a)};
        const vec3 q{radius*std::cos(beta),0,radius*std::sin(beta)};
        b.triangle(top,q,p);
        b.triangle(bottom,p,q);
    }
    return b.finish();
}

inline mesh_t tetrahedron() {
    Builder b;
    const vec3 p[4]={{0,0.5f,0},{-0.46f,-0.3f,0.34f},
                     {0.46f,-0.3f,0.34f},{0,-0.3f,-0.48f}};
    b.triangle(p[0],p[2],p[1]);
    b.triangle(p[0],p[3],p[2]);
    b.triangle(p[0],p[1],p[3]);
    b.triangle(p[1],p[2],p[3]);
    return b.finish();
}
inline std::array<mesh_t,count> create(mesh_t cube) {
    std::array<mesh_t,count> m{};
    m[0]=cube;
    m[1]=mesh_gen_sphere(1.0f,3);
    m[2]=tetrahedron();
    m[3]=bipyramid(4,0.5f,0.5f);
    // Counterclockwise outlines, with no mesh allocations while painting.
    m[4]=prism({{-0.16f,-0.5f},{0.16f,-0.5f},{0.16f,-0.16f},
       {0.5f,-0.16f},{0.5f,0.16f},{0.16f,0.16f},{0.16f,0.5f},
       {-0.16f,0.5f},{-0.16f,0.16f},{-0.5f,0.16f},
       {-0.5f,-0.16f},{-0.16f,-0.16f}},0.28f);
    std::vector<vec2> star;
    constexpr float pi=3.14159265358979323846f;
    for(int i=0;i<10;++i) {
        const float angle=pi*0.5f+i*pi/5.0f;
        const float radius=i%2?0.22f:0.5f;
        star.push_back({radius*std::cos(angle),radius*std::sin(angle)});
    }
    m[5]=prism(star,0.22f);
    std::vector<vec2> circle;
    for(int i=0;i<16;++i) {
        const float angle=2*pi*i/16;
        circle.push_back({0.5f*std::cos(angle),0.5f*std::sin(angle)});
    }
    m[6]=prism(circle,0.9f);
    m[7]=bipyramid(6,0.5f,0.5f);
    return m;
}
} // namespace voxen_shapes
