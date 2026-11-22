#version 450
layout(location=0) in vec4 vColor;
layout(location=1) in vec2 vDisk;
layout(location=0) out vec4 outColor;
void main() {
    float r2=dot(vDisk,vDisk);
    float core=exp(-19.0*r2);
    float halo=exp(-4.0*r2);
    float crossGlow=exp(-44.0*vDisk.x*vDisk.x)*exp(-3.8*vDisk.y*vDisk.y)+
                    exp(-44.0*vDisk.y*vDisk.y)*exp(-3.8*vDisk.x*vDisk.x);
    float alpha=vColor.a*(0.68*core+0.20*halo+0.18*crossGlow);
    outColor=vec4(mix(vColor.rgb,vec3(1.0),core*0.65),clamp(alpha,0.0,0.95));
}
