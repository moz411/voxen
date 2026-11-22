#version 450
layout(location=0) in vec4 vColor;
layout(location=1) in vec3 vRibbon;
layout(location=0) out vec4 outColor;
void main() {
    float r=abs(vRibbon.x);
    float core=1.0-smoothstep(0.06,0.26,r);
    float halo=1.0-smoothstep(0.15,1.0,r);
    float flow=0.84+0.16*sin(vRibbon.z*5.0+vRibbon.y*8.0);
    vec3 hot=mix(vColor.rgb,vec3(1.0),core*0.66);
    outColor=vec4(hot,clamp(vColor.a*(0.36*halo+0.64*core)*flow,0.0,0.95));
}
