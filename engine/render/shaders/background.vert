#version 450
// Per-eye world-space ray, reconstructed from OpenXR asymmetric FOV.
// The push-constant matrix columns encode eye right/up/back and tangent bounds.
layout(push_constant) uniform Push {
    mat4 mvp;
    vec4 parameters;
} pc;
layout(location=0) out vec3 localColor;
void main() {
    vec2 uv=vec2(float((gl_VertexIndex<<1)&2),float(gl_VertexIndex&2));
    vec2 ndc=uv*2.0-1.0;
    float tx=mix(pc.mvp[3].x,pc.mvp[3].y,ndc.x*0.5+0.5);
    float ty=mix(pc.mvp[3].z,pc.mvp[3].w,ndc.y*0.5+0.5);
    vec3 ray=normalize(pc.mvp[0].xyz*tx + pc.mvp[1].xyz*ty - pc.mvp[2].xyz);
    localColor=ray*0.5+0.5;
    gl_Position=vec4(ndc,0.99999,1.0);
}
