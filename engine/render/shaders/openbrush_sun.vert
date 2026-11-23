#version 450
#extension GL_GOOGLE_include_directive : require
#include "openbrush_particle_anim.glsl"
layout(location=0) in vec3 inPosition;
layout(location=1) in vec3 inNormal;
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
layout(location=3) out vec3 vNormal;
layout(location=4) out float vSphere;
void main() {
    bool sphere=inUV.z<0.0;
    vec3 pos=sphere?inPosition:
        inPosition+inUV.w*(camera.cameraRight.xyz*inUV.x+camera.cameraUp.xyz*inUV.y);
    gl_Position=camera.viewProjection*vec4(brushMotion(pos,camera.settings,inUV.z),1.0);
    vDisk=inUV.xy;
    vColor=vec4(brushCycle(inColor.rgb,camera.settings),inColor.a);
    vTime=camera.settings.x+(sphere?0.0:inUV.z);
    vNormal=inNormal;
    vSphere=sphere?1.0:0.0;
}
