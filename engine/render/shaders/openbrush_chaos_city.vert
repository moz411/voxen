#version 450
#extension GL_GOOGLE_include_directive : require
#include "openbrush_particle_anim.glsl"
layout(location=0) in vec3 inPosition;
layout(location=1) in vec3 inNormal;
layout(location=2) in vec4 inColor;
layout(location=3) in vec4 inUV;
layout(push_constant) uniform CityCamera {
    mat4 viewProjection;
    vec4 cameraRight;
    vec4 cameraUp;
    vec4 settings;
} camera;
layout(location=0) out vec4 vColor;
layout(location=1) out vec3 vNormal;
void main() {
    vec3 p=brushMotion(inPosition,camera.settings,inUV.x+inUV.y);
    gl_Position=camera.viewProjection*vec4(p,1.0);
    vColor=vec4(brushCycle(inColor.rgb,camera.settings),inColor.a);
    vNormal=inNormal;
}
