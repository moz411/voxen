#version 450
layout(location=0) in vec4 vColor;
layout(location=1) in vec3 vRibbon;
layout(location=0) out vec4 outColor;
void main() {
    float r=abs(vRibbon.x);
    float core=exp(-84.0*r*r);
    float halo=exp(-4.8*r*r);
    float hue=fract(vRibbon.y*0.92-vRibbon.z*0.16);
    vec3 rainbow=0.48+0.52*cos(6.2831853*(hue+vec3(0.0,0.333333,0.666667)));
    vec3 spectral=mix(rainbow,clamp(rainbow+vColor.rgb*0.18,0.0,1.0),0.45);
    vec3 color=mix(spectral,vec3(1.0),core*0.82);
    float shimmer=0.84+0.16*sin(vRibbon.y*25.0-vRibbon.z*4.0);
    outColor=vec4(color,clamp(vColor.a*shimmer*(0.70*core+0.31*halo),0.0,0.95));
}
