#version 450
layout(location=0) in vec2 uv;
layout(location=1) in vec3 normal;
layout(location=0) out vec4 outColor;
layout(push_constant) uniform PushConstants { mat4 mvp; vec4 parameters; } pc;
// parameters: time, selection boost, shader preset, surface kind
void main() {
    float t=pc.parameters.x;
    float u=fract(uv.x);
    float v=uv.y;
    float mode=floor(pc.parameters.z+0.5);
    vec3 col;
    if(pc.parameters.w>1.5) {
        // Procedural backgrounds: animated, seamless cylinder-space patterns.
        float wave=sin(18.0*u+4.0*v+t*(0.3+mod(mode,4.0)*0.17));
        float bands=sin(22.0*v-12.0*u+t*0.47);
        float twist=sin(6.2831853*(u+0.2*sin(v*7.0+t*0.4))+t*0.19);
        float intensity=0.5+0.24*wave+0.16*bands+0.10*twist;
        col=(0.48+0.48*cos(6.2831853*(u*0.6+mode*0.071+
            vec3(0.0,0.33,0.67))))*intensity*1.45;
    } else {
        float stripes=0.5+0.5*cos(6.2831853*(u*3.0+t*0.035));
        vec3 base=0.45+0.45*cos(6.2831853*(u+vec3(0.0,0.33,0.67)+mode*0.08));
        col=base*(0.55+0.45*stripes);
    }
    float edge=1.0-smoothstep(0.0,0.055,min(v,1.0-v));
    float pulse=0.65+0.35*sin(t*2.4+6.2831853*u);
    float aura=pow(1.0-abs(dot(normalize(normal),vec3(0.0,0.0,1.0))),3.0);
    col+=vec3(0.15,0.65,1.0)*(edge*(0.65+0.55*pulse)+0.16*aura*pulse);
    outColor=vec4(col*pc.parameters.y,0.96);
}
