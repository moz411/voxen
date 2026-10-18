#version 450
// Step-by-step diagnostic: uses the same proxy geometry, OpenXR layers and
// push constant layout as Mandelbulb, but only interpolates local coordinates.
// No raymarching, loops, conditionals, NaN checks, discard or depth writes.
layout(location=0) in vec3 localPosition;
layout(location=0) out vec4 outColor;
layout(push_constant) uniform PushConstants {
    mat4 mvp;
    vec4 parameters;
    vec4 eyeLocal;
} pc;
void main(){
    vec3 localColor=clamp(localPosition*0.5+vec3(0.5),vec3(0.0),vec3(1.0));
    outColor=vec4(localColor,1.0);
}
