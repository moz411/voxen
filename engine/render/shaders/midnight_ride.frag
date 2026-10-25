#version 450
// MIDNIGHT RIDE: self-contained Vulkan interpretation of Wearily's
// Shadertoy sketch (2026-10-24). Original supplied code references missing
// rgb, SDF and noise helper definitions, so these are reconstructed here.
// A 2D synthwave vignette projected on tracked Meta room surfaces.
layout(location=0) in vec2 uv;
layout(location=0) out vec4 outColor;
layout(push_constant) uniform PushConstants {mat4 mvp;vec4 parameters;} pc;
const float PI=3.14159265359;
float hash21(vec2 p){return fract(sin(dot(p,vec2(127.1,311.7)))*43758.5453);}
float noise(vec2 p){
    vec2 i=floor(p),f=fract(p);f=f*f*(3.0-2.0*f);
    return mix(mix(hash21(i),hash21(i+vec2(1,0)),f.x),
               mix(hash21(i+vec2(0,1)),hash21(i+vec2(1,1)),f.x),f.y);
}
float fbm(vec2 p){
    float s=0.0,a=0.5;
    for(int i=0;i<4;i++){s+=a*noise(p);p*=2.03;a*=0.5;}
    return s;
}
float lineSeg(vec2 p,vec2 a,vec2 b,float w){
    vec2 q=p-a,v=b-a;
    return length(q-v*clamp(dot(q,v)/max(dot(v,v),0.00001),0.0,1.0))-w;
}
float circle(vec2 p,vec2 c,float r){return length(p-c)-r;}
float box(vec2 p,vec2 c,vec2 h){vec2 d=abs(p-c)-h;return length(max(d,0.0))+min(max(d.x,d.y),0.0);}
float mask(float d,float blur){return 1.0-smoothstep(-blur,blur,d);}
float rotNoise(vec2 p,float t){return hash21(floor(p*120.0)+floor(t*19.0));}
void main(){
    float t=pc.parameters.x*max(pc.parameters.y,0.01);
    vec2 p=(uv-0.5)*vec2(2.0,2.0);
    vec3 col=vec3(0.384,0.118,1.0);
    col*=0.78+0.22*smoothstep(-0.8,0.9,p.y);
    // Magenta striped moon.
    vec2 mp=p-vec2(0.0,0.36);
    float disk=mask(circle(mp,vec2(0.0,0.025),0.43),0.012);
    col=mix(col,vec3(1.0,0.125,0.365),disk);
    float stripe=smoothstep(0.16,0.12,fract((mp.y+t*0.1)*10.0));
    col=mix(col,vec3(0.796,0.059,0.961),disk*stripe*0.76);
    // Reflection in water below the bridge.
    float water=1.0-smoothstep(-0.23,-0.04,p.y);
    float wave=sin(p.y*100.0+t*60.0)*0.012+sin(p.y*46.0-t*9.0)*0.013;
    float reflected=mask(circle(vec2(p.x*0.48+wave,(p.y+0.78)*1.5),vec2(0),0.13),0.2);
    col=mix(col,vec3(1.0,0.125,0.365),water*reflected*0.65);
    col+=water*vec3(0.20,0.04,0.25)*pow(noise(vec2(p.x*21.0,p.y*110.0+t*2.0)),4.0);
    // Repeating dark island silhouettes.
    float coast=0.055*sin((p.x-t*0.52)*7.0)+0.032*sin((p.x-t*0.52)*17.0);
    col=mix(col,vec3(0.02),mask(p.y+0.28+coast,0.015)*0.90);
    // Bridge deck, horizontal guard rail and moving supports.
    float bridge=mask(box(p,vec2(0,-0.105),vec2(2.0,0.027)),0.006);
    bridge=max(bridge,mask(box(p,vec2(0,0.115),vec2(2.0,0.007)),0.004));
    float rail=fract(p.x*5.5-t*8.0)-0.5;
    bridge=max(bridge,mask(box(vec2(rail,p.y),vec2(0,0.035),vec2(0.018,0.10)),0.005));
    float support=fract(p.x-t*0.75)-0.5;
    bridge=max(bridge,mask(box(vec2(support,p.y),vec2(0,-0.31),vec2(0.035,0.20)),0.009));
    col=mix(col,vec3(0.02),bridge);
    // Motorcycle and two riders, reconstructed from elementary SDFs.
    vec2 b=(p-vec2(0.16,-0.06-sin(t)*0.007))/0.29;
    float rear=circle(b,vec2(-0.65,-0.20),0.34);
    float front=circle(b,vec2(0.73,-0.20),0.34);
    float bike=min(abs(rear)-0.062,abs(front)-0.062);
    bike=min(bike,lineSeg(b,vec2(-0.65,-0.2),vec2(-0.10,0.23),0.07));
    bike=min(bike,lineSeg(b,vec2(-0.10,0.23),vec2(0.73,-0.2),0.07));
    bike=min(bike,lineSeg(b,vec2(-0.65,-0.2),vec2(0.73,-0.2),0.045));
    bike=min(bike,box(b,vec2(-0.02,0.07),vec2(0.32,0.20)));
    bike=min(bike,lineSeg(b,vec2(0.73,-0.2),vec2(0.55,0.55),0.04));
    bike=min(bike,lineSeg(b,vec2(0.55,0.55),vec2(0.32,0.55),0.04));
    bike=min(bike,box(b,vec2(-0.42,0.31),vec2(0.34,0.045)));
    // Front rider leaning forward, passenger sitting upright.
    bike=min(bike,circle(b,vec2(-0.02,1.12),0.17));
    bike=min(bike,lineSeg(b,vec2(-0.02,0.93),vec2(-0.19,0.43),0.14));
    bike=min(bike,lineSeg(b,vec2(-0.10,0.80),vec2(0.50,0.53),0.065));
    bike=min(bike,lineSeg(b,vec2(-0.20,0.42),vec2(0.25,0.12),0.08));
    bike=min(bike,circle(b,vec2(-0.67,1.02),0.14));
    bike=min(bike,lineSeg(b,vec2(-0.68,0.86),vec2(-0.66,0.40),0.13));
    bike=min(bike,lineSeg(b,vec2(-0.63,0.62),vec2(-0.19,0.67),0.065));
    bike=min(bike,lineSeg(b,vec2(-0.65,0.40),vec2(-0.38,0.08),0.08));
    col=mix(col,vec3(0.01),mask(bike,0.027));
    float tail=mask(box(b,vec2(-0.92,0.13),vec2(0.05,0.035)),0.018);
    col=mix(col,vec3(1.0,0.05,0.11),tail);
    // Grain, dirt and occasional signal interference.
    float grime=pow(fbm(p*35.5+vec2(t*0.25,0)),9.0);
    col+=grime*bridge*0.35;
    float staticDot=step(0.997,rotNoise(p,t));
    col+=vec3(0.45)*staticDot;
    col=clamp(col,0.0,1.0);
    outColor=vec4(col,clamp(pc.parameters.w,0.0,0.6));
}
