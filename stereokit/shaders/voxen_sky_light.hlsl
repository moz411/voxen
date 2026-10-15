#include "stereokit.hlsli"
//--time = 0
//--strength = 0.25
//--mode = 0
float time;
float strength;
float mode;
struct vsIn { float4 pos:SV_Position; float3 norm:NORMAL0; float2 uv:TEXCOORD0; float4 col:COLOR0; };
struct psIn { float4 pos:SV_Position; float3 dir:TEXCOORD0; uint view_id:SV_RenderTargetArrayIndex; };
psIn vs(vsIn v,uint id:SV_InstanceID) {
    psIn o;
    o.view_id=id%sk_view_count;
    id/=sk_view_count;
    float4 w=mul(v.pos,sk_inst[id].world);
    o.pos=mul(w,sk_viewproj[o.view_id]);
    o.dir=normalize(v.pos.xyz);
    return o;
}
float4 ps(psIn input):SV_TARGET {
    float3 d=normalize(input.dir);
    float horizon=saturate(0.5+0.5*d.y);
    float angle=atan2(d.z,d.x);
    float3 c;
    if(mode < 0.5) {
        // Aurora: two oscillating luminous ribbons, no texture reads or loops.
        float wave=sin(angle*3.0+time*0.28)*0.16+sin(angle*7.0-time*0.12)*0.06;
        float band=exp(-24.0*abs(d.y-0.30-wave));
        float band2=exp(-35.0*abs(d.y-0.54+wave*0.6));
        c=lerp(float3(0.004,0.012,0.035),float3(0.02,0.06,0.11),horizon);
        c+=float3(0.06,0.86,0.49)*band*0.65+float3(0.20,0.35,0.97)*band2*0.45;
    } else {
        // Ember horizon: a few traveling sine ridges, never particle loops.
        float ridge=sin(angle*9.0+time*0.32)+sin(angle*17.0-time*0.14);
        float glow=exp(-12.0*abs(d.y-0.04-ridge*0.035));
        float sparks=pow(saturate(sin(angle*47.0+time*0.8)*sin(d.y*54.0-time*0.6)),16.0);
        c=lerp(float3(0.035,0.004,0.008),float3(0.10,0.025,0.02),horizon);
        c+=float3(1.0,0.20,0.035)*glow*0.65+float3(1.0,0.50,0.08)*sparks*0.17;
    }
    return float4(saturate(c),saturate(strength));
}
