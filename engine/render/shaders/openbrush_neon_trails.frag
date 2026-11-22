#version 450
layout(location=0) in vec4 vColor;
layout(location=1) in vec3 vRibbon;
layout(location=0) out vec4 outColor;
void main() {
    float r=abs(vRibbon.x);
    // Tube-like emissive core within a much wider additive gaussian halo.
    float core=exp(-88.0*r*r);
    float sheath=exp(-15.0*r*r);
    float halo=exp(-3.8*r*r);
    float wave=sin(vRibbon.y*30.0-vRibbon.z*4.2);
    float pulse=0.78+0.22*wave*wave;
    float alpha=vColor.a*(0.72*core+0.25*sheath+0.12*halo)*pulse;
    vec3 edge=mix(vec3(0.21,0.12,0.75),vColor.rgb,0.72);
    vec3 color=mix(edge,vec3(0.97,0.99,1.0),core*0.93);
    outColor=vec4(color,clamp(alpha,0.0,0.95));
}
