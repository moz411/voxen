#version 450
#extension GL_GOOGLE_include_directive : require
#include "openbrush_particle_anim.glsl"
layout(location=0) in vec3 inPosition;
layout(location=1) in vec3 inCenter;
layout(location=2) in vec4 inColor;
layout(location=3) in vec4 inUV;
layout(push_constant) uniform BubbleCamera {
    mat4 viewProjection;
    vec4 cameraRight;
    vec4 cameraUp;
    vec4 settings;
} camera;
layout(location=0) out vec4 vColor;
layout(location=1) out vec2 vDisk;
layout(location=2) out float vPhase;
void main() {
    float t=camera.settings.x;
    float phase=inUV.z;
    bool star=phase<0.0;
    float radius=inUV.w;
    vec3 offset=star ? vec3(0.0) : vec3(
        sin(t*0.8+phase)*0.045,
        sin(t*0.35+phase)*0.07+mod(t*0.012+phase*0.01,0.7),
        cos(t*0.65+phase)*0.035);
    vec3 right=camera.cameraRight.xyz;
    vec3 up=camera.cameraUp.xyz;
    float twinkle=star ? (0.8+0.25*sin(t*4.5+abs(phase)*3.1)) : 1.0;
    vec3 pos=inCenter+offset+radius*twinkle*(right*inUV.x+up*inUV.y);
    gl_Position=camera.viewProjection*vec4(brushMotion(pos,camera.settings,phase),1.0);
    vColor=vec4(brushCycle(inColor.rgb,camera.settings),inColor.a);
    vDisk=inUV.xy;
    vPhase=star ? phase : phase+t*0.7;
}
