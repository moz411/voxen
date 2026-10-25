#version 450
layout(location=0) in vec3 localColor;
layout(location=0) out vec4 outColor;
layout(push_constant) uniform PushConstants {
    mat4 mvp;
    vec4 parameters; // time, intensity, material, alpha
} pc;
void main() {
    float edge=pow(max(max(abs(localColor.x-.5),abs(localColor.y-.5)),abs(localColor.z-.5))*2.0,3.0);
    float glow=clamp(pc.parameters.y,0.0,1.0);
    vec3 steel=mix(vec3(.035,.055,.075),vec3(.18,.24,.29),edge);
    vec3 hot=vec3(.05,.75,1.0)*(0.18+1.9*glow);
    vec3 rgb=mix(steel,hot,clamp(pc.parameters.z,0.0,1.0));
    outColor=vec4(rgb,clamp(pc.parameters.w,0.0,1.0));
}
