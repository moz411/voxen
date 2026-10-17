#version 450
// IFS PHANTOM TUNNEL — adapted from user-supplied GLSL for Vulkan/OpenXR.
// Phantom Mode credited in original to aiekick: https://www.shadertoy.com/view/MtScWW
// 56 raymarch steps instead of 99 to keep Quest fragment workloads bounded.
layout(location=0) in vec2 uv;
layout(location=0) out vec4 outColor;
layout(push_constant) uniform PushConstants {mat4 mvp;vec4 parameters;} pc;
const float PI=3.14159265358979323846;
const float TAU=6.2831853071795864769;
mat2 rot(float a){float c=cos(a),s=sin(a);return mat2(c,s,-s,c);}
vec2 pmod(vec2 p,float r){
    float a=atan(p.x,p.y)+PI/r;
    float n=TAU/r;
    a=floor(a/n)*n;
    return p*rot(-a);
}
float boxSdf(vec3 p,vec3 b){
    vec3 d=abs(p)-b;
    return min(max(d.x,max(d.y,d.z)),0.0)+length(max(d,vec3(-0.1)));
}
float ifsBox(vec3 p,float time){
    for(int i=1;i<3;i++){
        p=abs(p)-1.0;
        p.xy*=rot(time*0.3);
        p.xz*=rot(time*0.1);
    }
    p.xz*=rot(time);
    return boxSdf(p,vec3(0.4,0.8,0.3));
}
float mapScene(vec3 p,float time){
    p.x=mod(p.x-5.0,10.0)-5.0;
    p.y=mod(p.y-5.0,10.0)-5.0;
    p.z=mod(p.z,16.0)-8.0;
    p.xy=pmod(p.xy,5.0);
    return ifsBox(p,time);
}
void main(){
    float time=pc.parameters.x*pc.parameters.y;
    vec2 p=uv*2.0;
    vec3 cameraPos=vec3(0.0,0.0,-3.0*time);
    vec3 cameraDir=vec3(0.0,0.0,-1.0);
    vec3 cameraUp=normalize(vec3(cos(time),1.0,0.0));
    vec3 cameraSide=normalize(cross(cameraDir,cameraUp));
    cameraUp=normalize(cross(cameraSide,cameraDir));
    vec3 ray=normalize(cameraSide*p.x+cameraUp*p.y+cameraDir);
    float acc=0.0,acc2=0.0,t=0.0;
    for(int i=0;i<56;i++){
        vec3 pos=cameraPos+ray*t;
        float dist=max(abs(mapScene(pos,time)),0.02);
        float intensity=exp(-dist*3.0);
        if(mod(length(pos)+24.0*time,30.0)<3.0){
            intensity*=2.0;
            acc2+=intensity;
        }
        acc+=intensity;
        t+=dist*0.5;
    }
    vec3 col=vec3(acc*0.01,acc*0.011+acc2*0.01,acc*0.012+acc2*0.015);
    // Preserve the existing BACKGROUNDS opacity contract.
    outColor=vec4(clamp(col,0.0,1.0),clamp((1.0-t*0.3)*pc.parameters.w,0.0,0.6));
}
