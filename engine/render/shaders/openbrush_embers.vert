#version 450
#extension GL_GOOGLE_include_directive : require
#include "openbrush_particle_anim.glsl"
layout(location=0) in vec3 inPosition;
layout(location=1) in vec3 inCenter;
layout(location=2) in vec4 inColor;
layout(location=3) in vec4 inUV;
layout(push_constant) uniform EmberCamera {
    mat4 viewProjection;
    vec4 cameraRight;
    vec4 cameraUp;
    vec4 settings;
} camera;
layout(location=0) out vec4 vColor;
layout(location=1) out vec2 vDisk;
void main() {
    float t=camera.settings.x;
    float phase=inUV.x;
    float cycle=2.0+mod(abs(phase),1.8);
    float rise=mod(t*0.32+phase*0.21,cycle);
    vec3 offset=vec3(
        sin(t*1.8+phase)*0.045,
        rise*0.24,
        cos(t*1.3+phase)*0.035);
    float flicker=0.52+0.48*pow(max(0.0,sin(t*13.0+phase*2.3)),2.0);
    float radius=inUV.w*(0.7+0.3*flicker);
    vec3 local=camera.cameraRight.xyz*inUV.y*radius +
               camera.cameraUp.xyz*inUV.z*radius*1.6;
    gl_Position=camera.viewProjection*vec4(brushMotion(inCenter+offset+local,camera.settings,phase),1.0);
    vColor=vec4(brushCycle(inColor.rgb*flicker,camera.settings),inColor.a*flicker*(1.0-rise/cycle));
    vDisk=inUV.yz;
}
