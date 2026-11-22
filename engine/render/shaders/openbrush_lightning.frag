#version 450
layout(location=0) in vec4 vColor;
layout(location=1) in vec4 vShape;
layout(location=0) out vec4 outColor;
void main() {
    float r=abs(vShape.x);
    float core=exp(-38.0*r*r);
    float halo=exp(-3.4*r*r);
    float level=vShape.z;
    float taper=1.0-smoothstep(0.75,1.0,vShape.y);
    float intensity=vColor.a*vShape.w*taper*(1.0-0.23*level);
    float alpha=intensity*(0.76*core+0.30*halo);
    vec3 violet=vec3(0.33,0.24,1.0);
    vec3 blue=mix(violet,max(vColor.rgb,vec3(0.18,0.48,0.95)),0.72);
    vec3 color=mix(blue,vec3(0.95,0.98,1.0),core*0.94);
    outColor=vec4(color,clamp(alpha,0.0,0.95));
}
