#version 450
layout(location=0) in vec3 position;
layout(location=1) in vec3 normal;
layout(location=2) in vec2 uv0;
layout(location=3) in vec4 color0;
layout(location=2) out vec4 vColor;
layout(location=0) out vec3 vNormal;
layout(location=1) out vec2 vUV;
layout(push_constant) uniform PushConstants {mat4 mvp;vec4 parameters;} pc;
void main(){gl_Position=pc.mvp*vec4(position,1.0);vNormal=normal;vUV=uv0;vColor=color0;}
