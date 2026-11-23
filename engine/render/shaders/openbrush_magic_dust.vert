#version 450
#extension GL_GOOGLE_include_directive : require
#include "openbrush_particle_anim.glsl"
layout(location=0) in vec3 inPosition;
layout(location=1) in vec3 inCenter;
layout(location=2) in vec4 inColor;
layout(location=3) in vec4 inUV;
layout(push_constant) uniform Camera {
    mat4 viewProjection;
    vec4 right;
    vec4 up;
    vec4 settings;
} camera;
layout(location=0) out vec4 vColor;
layout(location=1) out vec2 vDisk;
void main() {
    float t=camera.settings.x,phase=inUV.z;
    float cycle=2.8+fract(phase*0.13)*1.6;
    float age=mod(t*0.34+phase*0.29,cycle)/cycle;
    vec3 drift=vec3(sin(t*0.83+phase)*0.065,
        age*0.34+sin(t*1.24+phase*1.6)*0.04,
        cos(t*0.91+phase*1.2)*0.065);
    float twinkle=0.75+0.25*sin(t*6.0+phase*4.7);
    float radius=inUV.w*twinkle;
    vec3 offset=(camera.right.xyz*inUV.x+camera.up.xyz*inUV.y)*radius;
    gl_Position=camera.viewProjection*vec4(brushMotion(inCenter+drift+offset,camera.settings,phase),1.0);
    vColor=vec4(brushCycle(inColor.rgb,camera.settings),inColor.a*(1.0-age*0.85));
    vDisk=inUV.xy;
}
