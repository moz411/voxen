// Godot canvas_item cloudy skies adapted to equirectangular UV on a StereoKit sphere.
// Inspired by Drift: https://www.shadertoy.com/view/4tdSWr
float2 cloud_hash(float2 p) {
    p=float2(dot(p,float2(127.1,311.7)),dot(p,float2(269.5,183.3)));
    return -1.0+2.0*frac(sin(p)*43758.5453123);
}
float cloud_noise(float2 p) {
    const float K1=0.366025404, K2=0.211324865;
    float2 i=floor(p+(p.x+p.y)*K1);
    float2 a=p-i+(i.x+i.y)*K2;
    float2 o=a.x>a.y?float2(1,0):float2(0,1);
    float2 b=a-o+K2, c=a-1.0+2.0*K2;
    float3 h=max(0.5-float3(dot(a,a),dot(b,b),dot(c,c)),0.0);
    float3 n=h*h*h*h*float3(dot(a,cloud_hash(i)),dot(b,cloud_hash(i+o)),dot(c,cloud_hash(i+1.0)));
    return dot(n,float3(70,70,70));
}
float2 cloud_warp(float2 p) { return float2(1.6*p.x-1.2*p.y,1.2*p.x+1.6*p.y); }
float cloud_fbm(float2 p) {
    float total=0.0,amp=0.1;
    [unroll] for(int i=0;i<7;++i) {total+=cloud_noise(p)*amp;p=cloud_warp(p);amp*=0.4;}
    return total;
}
float3 environment_clouds(float2 uv,float t) {
    float2 p=float2(uv.x*2.0,uv.y);
    float tm=t*0.01, q=cloud_fbm(p*1.1*0.5);
    float2 u=p*1.1+q-tm;
    float r=0,weight=0.8;
    [unroll] for(int i=0;i<8;++i) {r+=abs(weight*cloud_noise(u));u=cloud_warp(u)+tm;weight*=0.7;}
    u=p*1.1+q-tm;
    float f=0;weight=0.7;
    [unroll] for(int i=0;i<8;++i) {f+=weight*cloud_noise(u);u=cloud_warp(u)+tm;weight*=0.6;}
    f*=r+f;
    u=p*2.2+q-tm*2;
    float c=0;weight=0.4;
    [unroll] for(int i=0;i<7;++i) {c+=weight*cloud_noise(u);u=cloud_warp(u)+tm*2;weight*=0.6;}
    u=p*3.3+q-tm*3;
    float c1=0;weight=0.4;
    [unroll] for(int i=0;i<7;++i) {c1+=abs(weight*cloud_noise(u));u=cloud_warp(u)+tm*3;weight*=0.6;}
    c+=c1;
    float3 sky=lerp(float3(0.4,0.7,1),float3(0.2,0.4,0.6),uv.y);
    float3 cloud=float3(1.1,1.1,0.9)*saturate(0.5+0.3*c);
    f=0.2+8.0*f*r;
    return lerp(sky,saturate(0.5*sky+cloud),saturate(f+c));
}
