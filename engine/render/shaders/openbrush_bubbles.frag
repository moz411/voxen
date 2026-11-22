#version 450
layout(location=0) in vec4 vColor;
layout(location=1) in vec2 vDisk;
layout(location=2) in float vPhase;
layout(location=0) out vec4 outColor;
void main() {
    float r=length(vDisk);
    if(r>1.0)discard;
    float rim=1.0-smoothstep(0.93,1.0,r);
    float ring=smoothstep(0.58,0.88,r)*(1.0-smoothstep(0.95,1.0,r));
    float highlight=exp(-38.0*dot(vDisk-vec2(-0.32,0.35),vDisk-vec2(-0.32,0.35)));
    vec3 rainbow=0.5+0.5*cos(vPhase+vec3(0.0,2.1,4.2)+r*9.0);
    vec3 color=mix(vec3(0.55,0.78,1.0),rainbow,0.62)+highlight*vec3(0.7,0.8,0.9);
    float alpha=vColor.a*(0.11+0.72*ring+0.55*highlight)*rim;
    outColor=vec4(color,clamp(alpha,0.0,0.92));
}
