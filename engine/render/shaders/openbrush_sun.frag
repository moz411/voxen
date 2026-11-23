#version 450
// Single-pass high-energy sun: incandescent photosphere, animated
// granulation, fiery corona and broad analytic bloom. No postprocess required.
layout(location=0) in vec2 vDisk;
layout(location=1) in vec4 vColor;
layout(location=2) in float vTime;
layout(location=0) out vec4 outColor;
float hash21(vec2 p){return fract(sin(dot(p,vec2(127.1,311.7)))*43758.5453);}
float noise(vec2 p) {
    vec2 i=floor(p),f=fract(p);f=f*f*(3.0-2.0*f);
    return mix(mix(hash21(i),hash21(i+vec2(1,0)),f.x),
               mix(hash21(i+vec2(0,1)),hash21(i+vec2(1,1)),f.x),f.y);
}
void main() {
    float r=length(vDisk);
    if(r>1.4142)discard;
    float theta=atan(vDisk.y,vDisk.x);
    float t=vTime;
    float n=noise(vDisk*13.0+vec2(t*.37,-t*.21));
    float n2=noise(vDisk*29.0+vec2(-t*.51,t*.26));
    float flare=noise(vec2(theta*8.0+t*.22,r*7.0-t*.47));
    float limb=.37+.045*(n-.5)+.065*(flare-.5);
    float core=1.0-smoothstep(limb-.07,limb+.025,r);
    float granulation=.82+.18*n+.09*n2;
    vec3 whiteHot=vec3(2.9,2.35,1.35);
    vec3 orange=vec3(2.2,.65,.075);
    vec3 surface=mix(whiteHot,orange,smoothstep(.05,.43,r))*granulation;
    float corona=exp(-5.2*max(r-limb,0.0));
    float rays=pow(max(0.0,flare),3.0)*exp(-2.7*max(r-limb,0.0));
    float halo=exp(-2.1*r*r);
    vec3 color=surface*core+vec3(2.5,.65,.06)*(corona*.65+rays*.9)
        +vec3(1.4,.25,.015)*halo*.35;
    float alpha=clamp(core+corona*.52+rays*.38+halo*.14,0.0,.98)*vColor.a;
    outColor=vec4(color,alpha);
}
