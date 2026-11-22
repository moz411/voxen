#version 450
layout(location=0) in vec4 vColor;
layout(location=1) in vec2 vDisk;
layout(location=2) in float vPhase;
layout(location=0) out vec4 outColor;
void main() {
    float r=length(vDisk);
    float rim=smoothstep(0.62,0.98,r);
    float highlight=pow(max(0.0,1.0-length(vDisk-vec2(-0.33,0.38))*1.8),9.0);
    vec3 iridescent=0.5+0.5*cos(vPhase+vec3(0.0,2.1,4.2)+r*8.0);
    vec3 color=mix(vColor.rgb,iridescent,0.48*rim)+highlight*vec3(0.8,0.9,1.0);
    float alpha=vColor.a*(0.07+0.64*rim+0.25*highlight);
    outColor=vec4(color,clamp(alpha,0.0,0.85));
}
