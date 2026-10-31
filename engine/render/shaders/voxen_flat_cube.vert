#version 450
// Draw procedurally shaded 'flat' Voxens on six faces of a real local cube.
// Uses identical push constants and 2D UV convention as fractal.vert.
// No vertex buffer; one indexed-by-gl_VertexIndex cube, 36 vertices.
layout(push_constant) uniform PushConstants { mat4 mvp; vec4 parameters; } pc;
layout(location=0) out vec2 uv;
const vec3 corners[8]=vec3[8](
 vec3(-1.,-1.,-1.), vec3(1.,-1.,-1.), vec3(1.,1.,-1.), vec3(-1.,1.,-1.),
 vec3(-1.,-1.,1.), vec3(1.,-1.,1.), vec3(1.,1.,1.), vec3(-1.,1.,1.));
const int indices[36]=int[36](
 0,2,1, 0,3,2, 4,5,6, 4,6,7,
 0,1,5, 0,5,4, 3,7,6, 3,6,2,
 0,4,7, 0,7,3, 1,2,6, 1,6,5
);
void main(){
 int f=gl_VertexIndex/6;
 vec3 p=corners[indices[gl_VertexIndex]];
 // The first 2 faces use XY; next 2 use XZ, last 2 YZ.
 if(f<2)uv=p.xy;
 else if(f<4)uv=p.xz;
 else uv=p.zy;
 gl_Position=pc.mvp*vec4(p,1.);
}
