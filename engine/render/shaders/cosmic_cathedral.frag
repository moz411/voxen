#version 450
// Cosmic Cathedral: animated equirectangular sky overlay.
// Requires a panorama sampler and a time push constant (integration pending).
layout(set=0,binding=0) uniform sampler2D panorama;
layout(push_constant) uniform Params {
    vec4 orientation;
    vec4 tangents;
    vec4 controls; // yaw, horizontal FOV, vertical FOV, fade
    vec4 animation; // time seconds, nebula strength, star strength, meteor strength
} pc;
layout(location=0) in vec2 clipXY;
layout(location=0) out vec4 outColor;
const float PI=3.141592653589793;
vec3 rotateQ(vec3 v,vec4 q) {return v+2.0*cross(q.xyz,cross(q.xyz,v)+q.w*v);}
float hash21(vec2 p) {return fract(sin(dot(p,vec2(127.1,311.7)))*43758.5453);}
float noise2(vec2 p) {
    vec2 i=floor(p), f=fract(p); f=f*f*(3.0-2.0*f);
    return mix(mix(hash21(i),hash21(i+vec2(1,0)),f.x),
               mix(hash21(i+vec2(0,1)),hash21(i+vec2(1,1)),f.x),f.y);
}
float stars(vec2 uv,float t) {
    vec2 grid=uv*vec2(950.0,475.0);
    vec2 cell=floor(grid), local=fract(grid)-0.5;
    float seed=hash21(cell);
    vec2 pos=vec2(hash21(cell+13.1),hash21(cell+51.7))-0.5;
    float d=length(local-pos*0.75);
    float point=1.0-smoothstep(0.008,0.065,d);
    float twinkle=0.78+0.22*sin(t*(0.35+seed*1.2)+seed*6.283);
    return point*step(0.987,seed)*twinkle;
}
float meteor(vec2 uv,float t) {
    // Periodic, deterministic streak, no temporal discontinuity in static stars.
    float phase=mod(t,19.0);
    float active=smoothstep(1.0,1.35,phase)*(1.0-smoothstep(2.1,2.55,phase));
    vec2 head=vec2(0.22+0.32*(phase-1.0),0.19+0.11*(phase-1.0));
    vec2 d=uv-head;
    float along=dot(d,normalize(vec2(1.0,0.35)));
    float across=abs(dot(d,normalize(vec2(-0.35,1.0))));
    return active*exp(-across*420.0)*smoothstep(-0.23,-0.005,along)*(1.0-smoothstep(-0.005,0.012,along));
}
void main() {
    vec2 screenUV=clipXY*0.5+0.5;
    vec3 ray=normalize(vec3(mix(pc.tangents.x,pc.tangents.y,screenUV.x),
                            mix(pc.tangents.z,pc.tangents.w,screenUV.y),-1.0));
    vec3 direction=rotateQ(ray,normalize(pc.orientation));
    float lon=atan(direction.x,-direction.z)-pc.controls.x;
    lon=mod(lon+PI,2.0*PI)-PI;
    float lat=asin(clamp(direction.y,-1.0,1.0));
    float hfov=clamp(pc.controls.y,0.1,2.0*PI), vfov=clamp(pc.controls.z,0.1,PI);
    vec2 uv=vec2(lon/hfov+0.5,0.5-lat/vfov);
    bool inside=all(greaterThanEqual(uv,vec2(0)))&&all(lessThanEqual(uv,vec2(1)));
    vec3 base=inside?texture(panorama,uv).rgb:vec3(1);
    float t=pc.animation.x;
    // Slow, low-frequency luminance variation over the original galactic panorama.
    float n=noise2(uv*vec2(7,4)+vec2(t*0.008,-t*0.004));
    float n2=noise2(uv*vec2(17,9)-vec2(t*0.014,t*0.006));
    float cloud=(n*0.65+n2*0.35-0.5)*clamp(pc.animation.y,0.0,1.0);
    vec3 tint=mix(vec3(0.12,0.3,0.85),vec3(0.78,0.18,0.82),n);
    vec3 color=base+cloud*tint*0.26;
    // Stars stay locked to the panorama, avoiding head-motion-dependent shimmer.
    float star=inside?stars(uv,t):0.0;
    color+=vec3(0.55,0.75,1.0)*star*clamp(pc.animation.z,0.0,2.0);
    color+=vec3(0.75,0.85,1.0)*meteor(uv,t)*clamp(pc.animation.w,0.0,2.0);
    float fade=smoothstep(0.0,1.0,clamp(pc.controls.w,0.0,1.0));
    outColor=vec4(mix(vec3(1),max(color,vec3(0)),fade),1);
}
