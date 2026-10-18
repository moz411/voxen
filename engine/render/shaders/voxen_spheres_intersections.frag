#version 450
// SPHERES INTERSECTIONS — procedural approximation of Higgsas's
// Blender Geometry Nodes composition (BlenderArtists post #408).
// Raymarched smoothly-intersecting sphere cells; no Blender runtime dependency.
layout(location=0) in vec2 uv;
layout(location=0) out vec4 outColor;
layout(push_constant) uniform PushConstants {mat4 mvp; vec4 parameters;} pc;
// x=global time, y=speed, z=hue, w=packing / relaxation
float hash(vec2 p){return fract(sin(dot(p,vec2(127.1,311.7)))*43758.5453123);}
float smin(float a,float b,float k){
    float h=max(k-abs(a-b),0.0)/k;
    return min(a,b)-h*h*k*0.25;
}
float sceneSdf(vec3 p,float t){
    float d=10.0;
    float density=clamp(pc.parameters.w,0.3,2.0);
    // Deterministic, relaxed-looking staggered packing of different radii.
    for(int j=-1;j<=1;++j){
        for(int i=-1;i<=1;++i){
            vec2 cell=vec2(float(i),float(j));
            float n=hash(cell);
            vec2 center=(cell+vec2(0.18*sin(n*6.283+0.3*t),0.15*cos(n*11.0+0.2*t)))*0.49;
            float radius=mix(0.22,0.36,n);
            radius*=mix(0.86,1.12,density*0.5);
            vec3 q=p-vec3(center.x,center.y,0.05*sin(n*7.0+t*0.28));
            float sphere=length(q)-radius;
            d=smin(d,sphere,0.022);
        }
    }
    return d;
}
vec3 normalAt(vec3 p,float t){
    float e=0.005;
    return normalize(vec3(
        sceneSdf(p+vec3(e,0,0),t)-sceneSdf(p-vec3(e,0,0),t),
        sceneSdf(p+vec3(0,e,0),t)-sceneSdf(p-vec3(0,e,0),t),
        sceneSdf(p+vec3(0,0,e),t)-sceneSdf(p-vec3(0,0,e),t)));
}
void main(){
    vec2 xy=uv*1.18;
    if(max(abs(xy.x),abs(xy.y))>1.02)discard;
    float t=pc.parameters.x*pc.parameters.y;
    vec3 ro=vec3(xy,0.9);
    vec3 rd=vec3(0.0,0.0,-1.0);
    float z=0.0;
    bool hit=false;
    vec3 p;
    for(int step=0;step<32;++step){
        p=ro+rd*z;
        float d=sceneSdf(p,t);
        if(d<0.004){hit=true;break;}
        z+=max(d,0.008);
        if(z>1.5)break;
    }
    if(!hit)discard;
    vec3 n=normalAt(p,t);
    float diffuse=0.23+0.77*max(dot(n,normalize(vec3(-0.6,0.7,1.2))),0.0);
    vec3 base=0.35+0.35*cos(6.2831853*(pc.parameters.z+vec3(0.0,0.20,0.43)));
    vec3 color=base*diffuse+0.18*pow(max(n.z,0.0),8.0);
    outColor=vec4(color,0.95);
}
