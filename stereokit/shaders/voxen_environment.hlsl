#include "stereokit.hlsli"
//--time = 0
//--strength = 0.30
//--mode = 0
float time;
float strength;
float mode;
#include "materials/grass_wind.hlsli"
#include "materials/cloudy_skies.hlsli"
struct vsIn {
    float4 pos : SV_Position;
    float3 norm : NORMAL0;
    float2 uv : TEXCOORD0;
    float4 col : COLOR0;
};
struct psIn {
    float4 pos : SV_Position;
    float3 direction : TEXCOORD0;
    uint view_id : SV_RenderTargetArrayIndex;
};
psIn vs(vsIn input, uint id : SV_InstanceID) {
    psIn o;
    o.view_id = id % sk_view_count;
    id /= sk_view_count;
    float4 world = mul(input.pos, sk_inst[id].world);
    o.pos = mul(world, sk_viewproj[o.view_id]);
    o.direction = normalize(input.pos.xyz);
    return o;
}
float4 ps(psIn input) : SV_TARGET {
    float3 dir=normalize(input.direction);
    float az=atan2(dir.z,dir.x);
    float latitude=asin(clamp(dir.y,-1.0,1.0));
    float2 uv=float2(az/6.2831853+0.5,latitude/3.14159265+0.5);
    // Procedural panoramic environment, NOT the original Cyber Fuji 2020 asset.
    float mountain=0.05+0.25*exp(-pow((uv.x-0.64)*7.0,2.0));
    float skyline=step(uv.y,0.40+mountain);
    float neonGrid=pow(max(0.0,cos(uv.x*125.0+time*0.14)),22.0)
                  +pow(max(0.0,cos((uv.y-0.25)*155.0)),22.0);
    float sky=clamp(uv.y,0.0,1.0);
    float3 c=lerp(float3(0.20,0.02,0.36),float3(0.02,0.32,0.55),sky);
    c=lerp(c,float3(0.05,0.015,0.12),skyline);
    c+=float3(1.0,0.12,0.52)*neonGrid*0.16*(1.0-skyline);
    return float4(saturate(c),saturate(strength));
}
