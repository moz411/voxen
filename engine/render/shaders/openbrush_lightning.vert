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
layout(location=1) out vec4 vShape;
void main() {
    float time=camera.settings.x;
    float progress=inUV.x,level=floor(inUV.z),seed=fract(inUV.z);
    vec3 tangent=normalize(inPosition-inCenter);
    vec3 forward=normalize(cross(camera.right.xyz,camera.up.xyz));
    vec3 lateral=cross(tangent,forward);
    float lateralLength=length(lateral);
    lateral=lateralLength>0.001?lateral/lateralLength:camera.right.xyz;
    // Vertices move coherently within each branch, but remain attached at its root.
    float jitter=sin(progress*41.0+seed*13.0+time*23.0)*0.6+
                 sin(progress*89.0-seed*9.0-time*31.0)*0.24;
    float width=inUV.w*(1.0-progress*0.55);
    vec3 center=inCenter+lateral*jitter*width*2.7*progress;
    gl_Position=camera.viewProjection*vec4(brushMotion(center+lateral*inUV.y*width,camera.settings,seed),1.0);
    vColor=vec4(brushCycle(inColor.rgb,camera.settings),inColor.a);
    float cycle=fract(time*1.15+seed*0.12);
    // Main arc first, followed by delayed forks and micro-arcs.
    float front=clamp((cycle-level*0.13)*2.5,0.0,1.0);
    float reveal=1.0-smoothstep(front-0.08,front+0.035,progress);
    float decay=1.0-smoothstep(0.64,0.91,cycle);
    vShape=vec4(inUV.y,progress,level,reveal*decay);
}
