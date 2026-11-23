#version 450
layout(location=0) in vec4 vColor;
layout(location=1) in vec4 vShape;
layout(location=0) out vec4 outColor;
void main() {
    float r=abs(vShape.x);
    float core=exp(-55.0*r*r);
    float halo=exp(-3.8*r*r);
    float taper=1.0-smoothstep(0.76,1.0,vShape.y);
    float alpha=vColor.a*vShape.w*taper*(1.0-0.20*vShape.z)*
                (0.82*core+0.33*halo);
    vec3 cyan=mix(vec3(0.04,0.32,0.96),vec3(0.14,0.92,1.0),0.7);
    vec3 color=mix(cyan,vec3(1.0),core*0.96);
    outColor=vec4(color,clamp(alpha,0.0,0.96));
}
