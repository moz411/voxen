#version 450
// Shared procedural surface for 2D Voxen effects. No vertex buffer.
// An 80-byte legacy material push constant is followed by 16 bytes for
// geometry. shape.x: 0 cube, 1 UV sphere, 2 octahedron, 3 cone, 4 cylinder.
layout(push_constant) uniform PushConstants {
    mat4 mvp;
    vec4 parameters;
    vec4 shape;
} pc;
layout(location=0) out vec2 uv;

const vec3 corners[8]=vec3[8](
 vec3(-0.5,-0.5,-0.5), vec3(0.5,-0.5,-0.5),
 vec3(0.5,0.5,-0.5), vec3(-0.5,0.5,-0.5),
 vec3(-0.5,-0.5,0.5), vec3(0.5,-0.5,0.5),
 vec3(0.5,0.5,0.5), vec3(-0.5,0.5,0.5));
const int indices[36]=int[36](
 0,2,1,0,3,2, 4,5,6,4,6,7,
 0,1,5,0,5,4, 3,7,6,3,6,2,
 0,4,7,0,7,3, 1,2,6,1,6,5);
const float PI=3.141592653589793;
void main() {
    vec3 pos;
    int shape=int(pc.shape.x+0.5);
    if(shape==1 || shape==3 || shape==4) {
        // 32 longitude strips x 16 rings: sphere, tapered cone, cylinder.
        const int SLICES=32, STACKS=16;
        int cell=gl_VertexIndex/6;
        int corner=gl_VertexIndex%6;
        int x=cell%SLICES, y=cell/SLICES;
        int dx=(corner==1 || corner==2 || corner==4)?1:0;
        int dy=(corner==2 || corner==4 || corner==5)?1:0;
        float u=float(x+dx)/float(SLICES);
        float v=float(y+dy)/float(STACKS);
        float lon=2.0*PI*u;
        if(shape==1) {
            float lat=PI*v;
            pos=0.5*vec3(sin(lat)*cos(lon),cos(lat),sin(lat)*sin(lon));
        } else {
            float radius=shape==3?0.5*(1.0-v):0.5;
            pos=vec3(radius*cos(lon),v-0.5,radius*sin(lon));
        }
        uv=vec2(2.0*u-1.0,1.0-2.0*v);
    } else if(shape==2) {
        // Octahedron: 8 triangular faces, each with stable UV mapping.
        const int face=gl_VertexIndex/3;
        const int corner=gl_VertexIndex%3;
        float sx=(face&1)==0?1.0:-1.0;
        float sz=(face&2)==0?1.0:-1.0;
        float sy=(face&4)==0?1.0:-1.0;
        if(corner==0)pos=vec3(0.0,sy*0.65,0.0);
        else if(corner==1)pos=vec3(sx*0.65,0.0,0.0);
        else pos=vec3(0.0,0.0,sz*0.65);
        uv=corner==0?vec2(0.0,0.95):(corner==1?vec2(-0.95,-0.95):vec2(0.95,-0.95));
    } else {
        // A closed cube using all six sides, indexed explicitly.
        int face=gl_VertexIndex/6;
        pos=corners[indices[gl_VertexIndex]];
        if(face<2)uv=pos.xy*2.0;
        else if(face<4)uv=pos.xz*2.0;
        else uv=pos.zy*2.0;
    }
    gl_Position=pc.mvp*vec4(pos,1.0);
}
