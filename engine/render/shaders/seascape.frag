#version 450
/*
 * SEASCAPE — adapted from "Seascape" by Alexander Alekseev (TDM), 2014.
 * CC BY-NC-SA 3.0: attribution, non-commercial use, share alike.
 * https://creativecommons.org/licenses/by-nc-sa/3.0/
 * Port to Voxen Vulkan/OpenXR wall background. Reduced raymarch/normal
 * complexity for standalone Quest; no mouse input or supersampling.
 */
layout(location=0) in vec2 uv;
layout(location=0) out vec4 outColor;
layout(push_constant) uniform PushConstants {mat4 mvp;vec4 parameters;} pc;
const float PI=3.14159265359;
const int NUM_STEPS=20;
const int ITER_GEOMETRY=3;
const int ITER_FRAGMENT=4;
const float SEA_HEIGHT=0.6;
const float SEA_CHOPPY=4.0;
const float SEA_SPEED=0.8;
const float SEA_FREQ=0.16;
const vec3 SEA_BASE=vec3(0.0,0.09,0.18);
const vec3 SEA_WATER_COLOR=vec3(0.8,0.9,0.6)*0.6;
const mat2 octave_m=mat2(1.6,1.2,-1.2,1.6);
float seaTime;
mat3 fromEuler(vec3 ang){
    vec2 a1=vec2(sin(ang.x),cos(ang.x));
    vec2 a2=vec2(sin(ang.y),cos(ang.y));
    vec2 a3=vec2(sin(ang.z),cos(ang.z));
    mat3 m;
    m[0]=vec3(a1.y*a3.y+a1.x*a2.x*a3.x,a1.y*a2.x*a3.x+a3.y*a1.x,-a2.y*a3.x);
    m[1]=vec3(-a2.y*a1.x,a1.y*a2.y,a2.x);
    m[2]=vec3(a3.y*a1.x*a2.x+a1.y*a3.x,a1.x*a3.x-a1.y*a3.y*a2.x,a2.y*a3.y);
    return m;
}
float hash(vec2 p){return fract(sin(dot(p,vec2(127.1,311.7)))*43758.5453123);}
float noise(vec2 p){
    vec2 i=floor(p),f=fract(p),u=f*f*(3.0-2.0*f);
    return -1.0+2.0*mix(
        mix(hash(i),hash(i+vec2(1,0)),u.x),
        mix(hash(i+vec2(0,1)),hash(i+vec2(1,1)),u.x),u.y);
}
float diffuseLight(vec3 n,vec3 l,float p){return pow(dot(n,l)*0.4+0.6,p);}
float specularLight(vec3 n,vec3 l,vec3 e,float s){
    return pow(max(dot(reflect(e,n),l),0.0),s)*(s+8.0)/(PI*8.0);
}
vec3 skyColor(vec3 e){
    e.y=(max(e.y,0.0)*0.8+0.2)*0.8;
    return vec3(pow(1.0-e.y,2.0),1.0-e.y,0.6+(1.0-e.y)*0.4)*1.1;
}
float seaOctave(vec2 p,float choppy){
    p+=noise(p);
    vec2 wv=1.0-abs(sin(p)),swv=abs(cos(p));
    wv=mix(wv,swv,wv);
    return pow(1.0-pow(wv.x*wv.y,0.65),choppy);
}
float seaMap(vec3 p,int iterations){
    float freq=SEA_FREQ,amp=SEA_HEIGHT,choppy=SEA_CHOPPY;
    vec2 q=p.xz;q.x*=0.75;
    float h=0.0;
    for(int i=0;i<ITER_FRAGMENT;i++){
        if(i>=iterations)break;
        float d=seaOctave((q+seaTime)*freq,choppy);
        d+=seaOctave((q-seaTime)*freq,choppy);
        h+=d*amp;
        q*=octave_m;freq*=1.9;amp*=0.22;
        choppy=mix(choppy,1.0,0.2);
    }
    return p.y-h;
}
vec3 seaNormal(vec3 p,float eps){
    float d=seaMap(p,ITER_FRAGMENT);
    return normalize(vec3(seaMap(p+vec3(eps,0,0),ITER_FRAGMENT)-d,
                          eps,seaMap(p+vec3(0,0,eps),ITER_FRAGMENT)-d));
}
float traceSea(vec3 ori,vec3 dir,out vec3 p){
    float tm=0.0,tx=1000.0;
    float hx=seaMap(ori+dir*tx,ITER_GEOMETRY);
    if(hx>0.0){p=ori+dir*tx;return tx;}
    float hm=seaMap(ori,ITER_GEOMETRY);
    for(int i=0;i<NUM_STEPS;i++){
        float den=hm-hx;
        float tmid=mix(tm,tx,clamp(hm/(abs(den)<0.00001?0.00001:den),0.0,1.0));
        p=ori+dir*tmid;
        float hmid=seaMap(p,ITER_GEOMETRY);
        if(hmid<0.0){tx=tmid;hx=hmid;}
        else{tm=tmid;hm=hmid;}
        if(abs(hmid)<0.001)break;
    }
    return mix(tm,tx,clamp(hm/max(abs(hm-hx),0.00001),0.0,1.0));
}
vec3 seaColor(vec3 p,vec3 n,vec3 l,vec3 eye,vec3 dist){
    float fresnel=clamp(1.0-dot(n,-eye),0.0,1.0);
    fresnel=min(fresnel*fresnel*fresnel,0.5);
    vec3 reflected=skyColor(reflect(eye,n));
    vec3 refracted=SEA_BASE+diffuseLight(n,l,80.0)*SEA_WATER_COLOR*0.12;
    vec3 col=mix(refracted,reflected,fresnel);
    float dd=max(dot(dist,dist),0.001);
    col+=SEA_WATER_COLOR*(p.y-SEA_HEIGHT)*0.18*max(1.0-dd*0.001,0.0);
    col+=vec3(specularLight(n,l,eye,600.0*inversesqrt(dd)));
    return col;
}
void main(){
    float t=pc.parameters.x*max(pc.parameters.y,0.01)*0.3;
    seaTime=1.0+pc.parameters.x*max(pc.parameters.y,0.01)*SEA_SPEED;
    vec2 q=uv*2.0-1.0;
    vec3 ang=vec3(sin(t*3.0)*0.1,sin(t)*0.2+0.3,t);
    vec3 ori=vec3(0.0,3.5,t*5.0);
    vec3 dir=normalize(vec3(q,-2.0));
    dir.z+=length(q)*0.14;
    dir=normalize(fromEuler(ang)*normalize(dir));
    vec3 p;
    traceSea(ori,dir,p);
    vec3 dist=p-ori;
    float eps=clamp(dot(dist,dist)*0.00015,0.002,0.18);
    vec3 n=seaNormal(p,eps);
    vec3 light=normalize(vec3(0,1,0.8));
    vec3 col=mix(skyColor(dir),seaColor(p,n,light,dir,dist),
                 pow(smoothstep(0.0,-0.02,dir.y),0.2));
    col=pow(max(col,vec3(0.0)),vec3(0.65));
    outColor=vec4(clamp(col,0.0,1.0),clamp(pc.parameters.w,0.0,0.6));
}
