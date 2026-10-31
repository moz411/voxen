#version 450
// Shared procedural surface for 2D Voxen effects. No vertex buffer.
// An 80-byte legacy material push constant is followed by 16 bytes for
// geometry. shape.x: 0 = cube (36 vertices), 1 = UV sphere (32*16*6).
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
    if(pc.shape.x >= 0.5) {
        // UV sphere with centered UV coordinates on each side of the seam.
        const int SLICES=32, STACKS=16;
        int cell=gl_VertexIndex/6;
        int corner=gl_VertexIndex%6;
        int x=cell%SLICES, y=cell/SLICES;
        int dx=(corner==1 || corner==2 || corner==4)?1:0;
        int dy=(corner==2 || corner==4 || corner==5)?1:0;
        float u=float(x+dx)/float(SLICES);
        float v=float(y+dy)/float(STACKS);
        float lon=2.0*PI*u;
        float lat=PI*v;
        pos=0.5*vec3(sin(lat)*cos(lon),cos(lat),sin(lat)*sin(lon));
        // Same [-1,1] material UV domain as the six cube faces.
        uv=vec2(2.0*u-1.0,1.0-2.0*v);
    } else {
        int face=gl_VertexIndex/6;
        pos=corners[indices[gl_VertexIndex]];
        if(face<2)uv=pos.xy*2.0;
        else if(face<4)uv=pos.xz*2.0;
        else uv=pos.zy*2.0;
    }
    gl_Position=pc.mvp*vec4(pos,1.0);
}
