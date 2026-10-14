#include "stereokit.hlsli"
//--color:color = 1,1,1,1
//--time = 0
//--mode = 0
float4 color;
float time;
float mode;
struct vsIn {
    float4 pos : SV_Position;
    float3 norm : NORMAL0;
    float2 uv : TEXCOORD0;
    float4 col : COLOR0;
};
struct psIn {
    float4 pos : SV_Position;
    float3 local : TEXCOORD0;
    float3 normal : TEXCOORD1;
    float4 tint : COLOR0;
    uint view_id : SV_RenderTargetArrayIndex;
};
psIn vs(vsIn input, uint id : SV_InstanceID) {
    psIn o;
    o.view_id = id % sk_view_count;
    id /= sk_view_count;
    float4 world = mul(input.pos, sk_inst[id].world);
    o.pos = mul(world, sk_viewproj[o.view_id]);
    o.local = input.pos.xyz; // stable voxel-local position, independent of world scale
    o.normal = normalize(mul(input.norm, (float3x3)sk_inst[id].world));
    o.tint = input.col * color * sk_inst[id].color;
    return o;
}

float2 rs_rot(float2 p, float a) {
    float s=sin(a), c=cos(a);
    return float2(c*p.x-s*p.y,s*p.x+c*p.y);
}
float2 rs_hash(float2 p) {
    return frac(sin(float2(dot(p,float2(127.1,311.7)),
                           dot(p,float2(269.5,183.3))))*43758.5453);
}
float rs_voronoi(float2 uv) {
    float2 cell=floor(uv*5.0), f=frac(uv*5.0);
    float d=100.0;
    [unroll] for(int y=-1;y<=1;y++) {
        [unroll] for(int x=-1;x<=1;x++) {
            float2 o=float2(x,y), h=rs_hash(cell+o);
            float2 q=o+0.5+0.5*float2(sin(h.x),cos(h.y));
            d=min(d,length(q-f));
        }
    }
    return d;
}
float rs_segment(float2 p,float2 a,float2 b) {
    float2 ab=b-a;
    return length(p-a-ab*saturate(dot(p-a,ab)/max(dot(ab,ab),1e-5)));
}
float3 rs_color(float2 uv,float t) {
    float r=length(uv);
    float3 core=1.0-exp(-float3(0.006,0.01,0.018)/max(abs(r-0.05),0.005));
    float3 edge=1.0-exp(-float3(0.01,0.01,0.05)/max(abs(r-0.30),0.005));
    float2 a=rs_rot(uv*3.0,-t*5.0);
    a=rs_rot(a,5.0*length(a));
    float2 b=rs_rot(uv*3.0,t*5.0);
    b=rs_rot(b,-5.0*length(b));
    float lines=(pow(rs_voronoi(a*1.3),2.5)+pow(rs_voronoi(b*0.5),3.0))
                *(1.0-smoothstep(0.29,0.32,r));
    float3 col=core+edge+lines*float3(0.1,0.4,0.8);
    float2 gl=abs(rs_rot(uv,t*0.3));
    float halo=0.0;
    [unroll] for(int i=1;i<=8;i++) {
        float k=(float)i;
        float offs=sin(t*15.0+frac(k*123.456+867.234)*5.0)*0.01;
        float h=0.32+frac(k*345.67+12.35)*0.1;
        float d=rs_segment(gl,float2(0,0.32+offs),float2(0,h+offs));
        halo+=1.0-exp(-0.0005/max(d,0.001));
        gl=rs_rot(gl,0.349066*frac(k*45.29));
    }
    return saturate(col*(1.0+halo*0.65));
}


float collection_hash(float2 seed) {
    seed=sin(seed*float2(12.45,56.43))*64.7+12.34;
    return frac(seed.x*seed.y);
}
// SHADERed EmCYTuRzv2: animated smooth value noise; six octaves.
// Source algorithm preserved, adapted from fullscreen UV to voxel-local coordinates.
float collection_noise(float2 uv) {
    float n=0.0, amp=1.0, den=0.0;
    [unroll] for(int i=0;i<6;i++) {
        float2 f=frac(uv), id=floor(uv);
        float2 w=f*f*(3.0-2.0*f);
        float bl=collection_hash(id), br=collection_hash(id+float2(1,0));
        float tl=collection_hash(id+float2(0,1)), tr=collection_hash(id+float2(1,1));
        n+=lerp(lerp(bl,br,w.x),lerp(tl,tr,w.x),w.y)*amp;
        den+=amp; amp*=0.5; uv*=2.0;
    }
    return n/den;
}
// SHADERed jHKMZig75S: layered moving colored particles, localized to each voxel face.
float3 collection_particles(float2 uv, float t) {
    float3 outc=float3(0.1,0.1,0.1);
    float2 p=uv*12.0;
    [loop] for(int j=0;j<10;j++) {
        float fj=j/10.0;
        float r1=(sin(fj*100.0+1.0)+1.0)*0.5;
        float r2=(sin(fj*100.0+7.0)+1.0)*0.5;
        float len=0.5+r2*0.7, maxSize=0.25+r2*0.4, minSize=0.15+r2*0.1;
        float period=lerp(3.0,10.0,r1);
        float phase=frac((t+3.141592+r2*13.0)/period);
        float x=lerp(-6.0-maxSize,6.0+len+minSize,phase);
        [unroll] for(int i=0;i<8;i++) {
            float fi=i/8.0;
            float2 q=float2(x-fi*len,-6.0+12.0*fj+0.6);
            q.y+=0.2*sin(5.0*q.x)+0.6*cos(2.0*q.x)-0.1*cos(-5.0*q.x);
            float radius=lerp(maxSize,minSize,fi);
            float mask=1.0-smoothstep(radius-0.02,radius+0.02,length(p.yx-q));
            float3 tint=float3(lerp(0.1,0.9,r1),lerp(0.4,0.8,r2),fi)*(1.0-0.8*fi);
            outc=lerp(outc,tint,mask);
        }
    }
    return outc;
}

float4 ps(psIn input) : SV_TARGET {
    float3 p = input.local;
    float t = time;
    float3 c = input.tint.rgb;
    if (mode > 0.5 && mode < 1.5) { // neon grid
        float3 g = abs(frac(p * 9.0 + 0.5) - 0.5);
        float grid_line = 1.0 - smoothstep(0.035, 0.095, min(g.x, min(g.y,g.z)));
        c = c * (0.45 + 1.15 * grid_line) + grid_line * 0.12;
    } else if (mode > 1.5 && mode < 2.5) { // plasma
        float v = sin(p.x*16+t*2)+sin(p.y*19-t*3)+sin(p.z*13+t*1.6);
        c = 0.55+0.45*cos(v*2.1+float3(0,2.1,4.2));
    } else if (mode > 2.5 && mode < 3.5) { // hologram scan lines
        float scan = 0.55+0.45*sin(p.y*110-t*11);
        c = float3(0.08,0.70,1.0) * scan;
    } else if (mode > 3.5 && mode < 4.5) { // lava
        float f=sin(p.x*16+sin(p.y*13+t*2.5))+sin(p.z*17-t*1.7);
        c=float3(1.0, 0.15+0.32*saturate(f+0.5), 0.015);
    } else if (mode > 7.5) { // animated noise from EmCYTuRzv2
        float n=collection_noise(p.xy*12.0-float2(t,0.0));
        c=float3(n,n,n);
    } else if (mode > 6.5) { // moving particles from jHKMZig75S
        c=collection_particles(p.xy,t);
    } else if (mode > 5.5) { // rasengan
        float3 n=abs(normalize(input.normal));
        float2 uv=n.x>n.y && n.x>n.z ? p.yz : (n.y>n.z ? p.xz : p.xy);
        c=rs_color(uv,t);
    } else if (mode > 4.5) { // ice
        float crack=pow(abs(sin((p.x+p.z)*32+sin(p.y*19+t))),12);
        c=float3(0.2,0.68,1.0)*(0.7+0.3*crack);
    }
    float light = 0.55 + 0.45*saturate(dot(normalize(input.normal),normalize(float3(0.35,0.8,-0.4))));
    return float4(saturate(c*light),input.tint.a);
}
