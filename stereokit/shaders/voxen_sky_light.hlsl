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
    } else if(mode < 1.5) {
        // Ember horizon: a few traveling sine ridges, never particle loops.
        float ridge=sin(angle*9.0+time*0.32)+sin(angle*17.0-time*0.14);
        float glow=exp(-12.0*abs(d.y-0.04-ridge*0.035));
        float sparks=pow(saturate(sin(angle*47.0+time*0.8)*sin(d.y*54.0-time*0.6)),16.0);
        c=lerp(float3(0.035,0.004,0.008),float3(0.10,0.025,0.02),horizon);
        c+=float3(1.0,0.20,0.035)*glow*0.65+float3(1.0,0.50,0.08)*sparks*0.17;
    }
    if(mode > 1.5 && mode < 2.5) {
        // Galaxy: a luminous Milky Way-like band plus sparse analytic stars.
        float band=exp(-14.0*abs(d.y+0.24*sin(angle*1.4+0.4)));
        float spiral=0.5+0.5*sin(angle*8.0+d.y*28.0-time*0.025);
        float cell=sin(angle*171.0)*sin(d.y*217.0);
        float stars=pow(saturate(cell),50.0);
        c=float3(0.002,0.003,0.014);
        c+=band*lerp(float3(0.09,0.05,0.25),float3(0.24,0.20,0.43),spiral)*0.95;
        c+=stars*float3(0.65,0.78,1.0)*0.48;
    } else if(mode > 2.5 && mode < 3.5) {
        // Aquatic: underwater turquoise gradient, shallow light shafts, drifting caustics.
        float depth=saturate(0.5-0.5*d.y);
        float caustic=sin(angle*15.0+time*0.16+sin(d.y*12.0))
                     *sin(d.y*19.0-angle*8.0-time*0.11);
        float shaft=pow(saturate(sin(angle*6.0+time*0.04)),12.0)*saturate(d.y+0.4);
        c=lerp(float3(0.01,0.11,0.19),float3(0.015,0.39,0.46),horizon);
        c+=float3(0.04,0.23,0.20)*saturate(caustic)*0.27;
        c+=float3(0.15,0.52,0.53)*shaft*0.32;
        c*=1.0-depth*0.25;
    } else if(mode > 3.5) {
        // Rainforest: layered canopy silhouettes, warm forest shafts and distant haze.
        float canopy=0.28+0.16*sin(angle*5.0)+0.08*sin(angle*13.0+0.5);
        float leaves=sin(angle*29.0+time*0.025)*sin(d.y*37.0-time*0.017);
        float tree=exp(-85.0*abs(sin(angle*5.0+0.25)))*saturate(0.8-d.y);
        float mist=exp(-11.0*abs(d.y+0.1));
        float sun=pow(saturate(sin(angle*7.0+time*0.02)),20.0)*saturate(d.y+0.1);
        float foliage=1.0-smoothstep(canopy-0.06,canopy+0.06,d.y);
        c=lerp(float3(0.13,0.29,0.17),float3(0.31,0.49,0.30),horizon);
        c=lerp(c,float3(0.014,0.072,0.031)*(0.84+0.16*leaves),foliage*0.80);
        c=lerp(c,float3(0.012,0.045,0.026),tree*0.76);
        c+=float3(0.23,0.27,0.13)*mist*0.17+float3(0.45,0.40,0.20)*sun*0.17;
    }
    return float4(saturate(c),saturate(strength));
}
