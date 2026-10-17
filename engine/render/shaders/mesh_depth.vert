#version 450
layout(location=0) in vec3 position;
layout(push_constant) uniform PushConstants {mat4 mvp;vec4 parameters;} pc;
void main(){gl_Position=pc.mvp*vec4(position,1.);}
