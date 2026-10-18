#version 450
layout(push_constant) uniform PushConstants {
    mat4 mvp;
    vec4 parameters;
    vec4 eyeLocal;
} pc;
layout(location=0) out vec3 localPosition;
const vec3 vertices[8]=vec3[](
vec3(-1,-1,-1),vec3(1,-1,-1),vec3(1,1,-1),vec3(-1,1,-1),
vec3(-1,-1,1),vec3(1,-1,1),vec3(1,1,1),vec3(-1,1,1));
const int indices[36]=int[](
0,2,1,0,3,2,4,5,6,4,6,7,0,1,5,0,5,4,
3,7,6,3,6,2,0,4,7,0,7,3,1,2,6,1,6,5);
void main(){
    localPosition=vertices[indices[gl_VertexIndex]];
    gl_Position=pc.mvp*vec4(localPosition,1.0);
}
