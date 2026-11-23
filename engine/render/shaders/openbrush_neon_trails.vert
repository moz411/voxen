#version 450
#extension GL_GOOGLE_include_directive : require
#include "openbrush_particle_anim.glsl"
layout(location=0) in vec3 inPosition; // opposite endpoint, for tangent
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
layout(location=1) out vec3 vRibbon;
void main() {
    vec3 tangent=normalize(inPosition-inCenter);
    vec3 forward=normalize(cross(camera.right.xyz,camera.up.xyz));
    vec3 lateral=cross(tangent,forward);
    float len=length(lateral);
    lateral=len>0.0001?lateral/len:camera.right.xyz;
    float breathing=0.94+0.06*sin(camera.settings.x*2.6+inUV.y*12.0);
    vec3 position=inCenter+lateral*inUV.x*inUV.w*breathing;
    gl_Position=camera.viewProjection*vec4(brushMotion(position,camera.settings,inUV.z),1.0);
    vColor=vec4(brushCycle(inColor.rgb,camera.settings),inColor.a);
    vRibbon=vec3(inUV.x,inUV.y,inUV.z+camera.settings.x*2.8);
}
