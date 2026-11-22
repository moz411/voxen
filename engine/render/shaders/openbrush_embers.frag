#version 450
// Hot center and decaying falloff; kept very small on GPU.
layout(location=0) in vec4 vColor;
layout(location=1) in vec2 vDisk;
layout(location=0) out vec4 outColor;
void main() {
    float r2=dot(vDisk,vDisk);
    float core=exp(-13.0*r2);
    float halo=exp(-3.8*r2);
    float spark=exp(-36.0*vDisk.x*vDisk.x)*(1.0-smoothstep(0.25,1.0,abs(vDisk.y)));
    float alpha=clamp(vColor.a*(0.80*core+0.24*halo+0.28*spark),0.0,0.95);
    outColor=vec4(mix(vColor.rgb,vec3(1.0,0.93,0.64),core*0.65),alpha);
}
