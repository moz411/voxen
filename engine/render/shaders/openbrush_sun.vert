#version 450
#extension GL_GOOGLE_include_directive : require
#include "openbrush_particle_anim.glsl"
layout(location=0) in vec3 inPosition;
layout(location=1) in vec3 inCenter;
layout(location=2) in vec4 inColor;
layout(location=3) in vec4 inUV;
layout(push_constant) uniform SunCamera {
    mat4 viewProjection;
    vec4 cameraRight;
    vec4 cameraUp;
    vec4 settings;
} camera;
layout(location=0) out vec2 vDisk;
layout(location=1) out vec4 vColor;
layout(location=2) out float vTime;
void main() {
    vec3 pos=inCenter+inUV.w*(camera.cameraRight.xyz*inUV.x+camera.cameraUp.xyz*inUV.y);
    gl_Position=camera.viewProjection*vec4(brushMotion(pos,camera.settings,inUV.z),1.0);
    vDisk=inUV.xy;
    vColor=vec4(brushCycle(inColor.rgb,camera.settings),inColor.a);
    vTime=camera.settings.x+inUV.z;
}
