#version 450
// Log-polar rainbow spiral, adapted from the user-supplied Shadertoy.
// f() and sinebow() were missing from the snippet: defined locally.
// Fixed nonzero controls replace iMouse for standalone Quest playback.
layout(location=0) in vec2 uv;
layout(location=0) out vec4 outColor;
layout(push_constant) uniform PushConstants {mat4 mvp;vec4 parameters;} pc;
const float TAU=6.283185307179586;
vec3 sinebow(float t) {
    return 0.5 + 0.5*cos(TAU*t + vec3(0.0,2.0943951,4.1887902));
}
float f(float x) {
    // Bounded periodic wave; avoids the discontinuity of a bare angle.
    return sin(x);
}
void main() {
    vec2 xy=8.0*uv;
    float r=log(max(length(xy),0.0001));
    float a=atan(xy.y,xy.x);
    const float k=2.4;
    const float w=0.7;
    float time=pc.parameters.x*max(pc.parameters.y,0.01);
    float d=(f(a-r*k)/k+r)-time*0.5;
    d/=w;
    outColor=vec4(sinebow(d),clamp(pc.parameters.w,0.0,0.6));
}
