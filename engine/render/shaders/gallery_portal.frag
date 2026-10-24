#version 450
layout(location=0) in vec2 uv;
layout(location=0) out vec4 outColor;
layout(push_constant) uniform PushConstants {
    mat4 mvp;
    vec4 parameters; // x=time, y=hue offset, z=selection, w=opacity
} pc;

vec3 spectrum(float t) {
    return 0.55 + 0.45*cos(6.2831853*(t+vec3(0.0,0.33,0.67)));
}
void main() {
    float r=length(uv);
    // Lightweight procedural proxy: no saved scene payload is loaded.
    if(pc.parameters.z>=2.0) {
        float motif=floor(pc.parameters.z-2.0+0.01);
        float angle=atan(uv.y,uv.x);
        float petals=4.0+mod(motif,5.0);
        float edge=0.43+0.12*cos(petals*angle+pc.parameters.x*0.65);
        float body=1.0-smoothstep(edge-0.035,edge+0.035,r);
        float core=1.0-smoothstep(0.06,0.16,r);
        float filaments=0.5+0.5*cos(angle*petals*2.0-r*24.0+pc.parameters.x);
        vec3 color=spectrum(pc.parameters.y+motif*0.113+r*0.35+pc.parameters.x*0.03);
        float alpha=clamp(body*(0.48+0.4*filaments)+core*0.3,0.0,0.88);
        outColor=vec4(color*1.2,alpha*pc.parameters.w);
        return;
    }
    float theta=atan(uv.y,uv.x);
    float wave=0.013*sin(theta*12.0-pc.parameters.x*1.7);
    float ring=1.0-smoothstep(0.045,0.08,abs(r-0.73-wave));
    float outer=1.0-smoothstep(0.01,0.06,abs(r-0.89));
    float inner=1.0-smoothstep(0.02,0.065,abs(r-0.59));
    float pulse=0.78+0.22*sin(pc.parameters.x*2.4);
    float alpha=clamp((ring*0.85+outer*0.35+inner*0.22)*pulse,0.0,1.0);
    vec3 rgb=spectrum(theta/6.2831853+pc.parameters.x*0.07+pc.parameters.y);
    rgb=mix(rgb,vec3(0.25,0.95,1.0),0.22);
    outColor=vec4(rgb*1.35,alpha*pc.parameters.w);
}
