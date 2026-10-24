#version 450
// SOUND DEMO — Shadertoy SDF/reflection port for Vulkan/OpenXR.
// The source's iChannel3 font atlas and text overlay are intentionally omitted:
// Voxen wall shaders have no Shadertoy texture channels.
layout(location=0) in vec2 uv;
layout(location=0) out vec4 outColor;
layout(push_constant) uniform PushConstants { mat4 mvp; vec4 parameters; } pc;
vec2 rotate2(vec2 p,float a){return cos(a)*p+sin(a)*vec2(p.y,-p.x);}
float minkowski(vec3 v,float m){return pow(pow(v.x,m)+pow(v.y,m)+pow(v.z,m),1.0/m);}
vec3 crt(vec3 p){
    float r=0.5*6.2831853/6.0;
    float pa=r-atan(p.x,p.y);
    pa+=mod(pa,-2.0*r);
    return vec3(rotate2(p.xy,pa),p.z);
}
vec2 sdf(vec3 p,float t){
    vec3 v=p;
    v.xz=rotate2(v.xz,t);
    vec3 q=abs(crt(abs(p)-vec3(1.0,0.1,3.0+mod(floor(2.0*t),3.0))));
    float a=minkowski(abs(q-vec3(0.7+mod(floor(t),3.0),4.2,1.3)),1.55)-1.25;
    float b=minkowski(abs(v),3.05)-(0.9+fract(0.2*floor(t)));
    return a<b?vec2(a,3.0):vec2(b,5.0);
}
vec3 normalAt(vec3 p,float t){
    const float e=0.002;
    return normalize(vec3(
        sdf(p+vec3(e,0,0),t).x-sdf(p-vec3(e,0,0),t).x,
        sdf(p+vec3(0,e,0),t).x-sdf(p-vec3(0,e,0),t).x,
        sdf(p+vec3(0,0,e),t).x-sdf(p-vec3(0,0,e),t).x));
}
void main(){
    float t=pc.parameters.x*pc.parameters.y;
    vec2 st=uv*2.0-1.0;
    vec3 ro=vec3(fract(t),fract(t),7.0);
    vec3 gc=normalize(-ro);
    vec3 u=normalize(cross(gc,vec3(0,1,0)));
    vec3 v=normalize(cross(u,gc));
    u*=tan(1.2);
    v*=tan(1.2);
    vec3 rd=normalize(gc+st.x*u+st.y*v);
    vec3 color=vec3(1.0);
    float h=1.0;
    // Limit to 24 steps on Quest: 3D SDF plus normals is expensive.
    for(int i=0;i<24;i++){
        vec3 p=ro+rd*h;
        vec2 d=sdf(p,t);
        if(abs(d.x)<0.002){
            vec3 n=normalAt(p,t);
            float incidence=clamp(dot(-rd,n),0.0,1.0);
            float fr=d.y<4.0?sqrt(max(1.0-incidence,0.0)):1.0-incidence;
            color*=max(fr,0.02);
            ro=p+n*(d.y<4.0?0.01:0.002);
            rd=reflect(rd,n);
            h=0.0;
        }
        h+=max(d.x,0.002);
        if(h>25.0)break;
    }
    outColor=vec4(clamp(color,0.0,1.0),clamp(pc.parameters.w,0.0,0.6));
}
