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


// Shader dispatch retains numerical IDs for existing scene files.
// Each procedural material has its own HLSL implementation.
#include "materials/rasengan.hlsli"
#include "materials/noise.hlsli"
#include "materials/particles.hlsli"
float4 ps(psIn input) : SV_TARGET {
    float3 p=input.local;
    float t=time;
    float3 c=input.tint.rgb;
    if (mode > 7.5) {
        float n=collection_noise(p.xy*12.0-float2(t,0.0));
        c=float3(n,n,n);
    } else if (mode > 6.5) {
        c=collection_particles(p.xy,t);
    } else if (mode > 5.5) {
        float3 n=abs(normalize(input.normal));
        float2 uv=n.x>n.y && n.x>n.z ? p.yz : (n.y>n.z ? p.xz : p.xy);
        c=rs_color(uv,t);
    } else if (mode > 4.5) {
        float crack=pow(abs(sin((p.x+p.z)*32+sin(p.y*19+t))),12);
        c=float3(0.2,0.68,1.0)*(0.7+0.3*crack);
    } else if (mode > 3.5) {
        float f=sin(p.x*16+sin(p.y*13+t*2.5))+sin(p.z*17-t*1.7);
        c=float3(1.0,0.15+0.32*saturate(f+0.5),0.015);
    } else if (mode > 2.5) {
        float scan=0.55+0.45*sin(p.y*110-t*11);
        c=float3(0.08,0.70,1.0)*scan;
    } else if (mode > 1.5) {
        float v=sin(p.x*16+t*2)+sin(p.y*19-t*3)+sin(p.z*13+t*1.6);
        c=0.55+0.45*cos(v*2.1+float3(0,2.1,4.2));
    } else if (mode > 0.5) {
        float3 g=abs(frac(p*9.0+0.5)-0.5);
        float grid_line=1.0-smoothstep(0.035,0.095,min(g.x,min(g.y,g.z)));
        c=c*(0.45+1.15*grid_line)+grid_line*0.12;
    }
    float light=0.55+0.45*saturate(dot(normalize(input.normal),normalize(float3(0.35,0.8,-0.4))));
    return float4(saturate(c*light),input.tint.a);
}
