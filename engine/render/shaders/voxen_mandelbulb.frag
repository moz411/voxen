#version 450
// Temporary diagnostic only: if this is invisible, the Vulkan proxy geometry,
// transform, or command submission is at fault (not Mandelbulb raymarching).
layout(location=0) in vec3 localPosition;
layout(location=0) out vec4 outColor;
layout(push_constant) uniform PushConstants {mat4 mvp;vec4 parameters;vec4 eyeLocal;} pc;
void main(){
    outColor=vec4(1.0,0.0,0.8,1.0);
}
