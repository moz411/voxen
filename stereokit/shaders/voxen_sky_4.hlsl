#include "stereokit.hlsli"
//--time = 0
//--strength = 0.25
float time;
float strength;
static const float mode = 4.0;
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
// Matrix-style glyph rain: analytic 3-stroke symbols, no texture/particle loops.
float matrix_hash(float2 p) {
    return frac(sin(dot(p,float2(127.1,311.7)))*43758.5453);
}
float matrix_segment(float2 p,float2 a,float2 b,float width) {
    float2 v=b-a;
    float t=saturate(dot(p-a,v)/dot(v,v));
    float2 q=p-a-v*t;
    return 1.0-smoothstep(width,width+0.07,length(q));
}
float3 matrix_rain(float angle,float height,float t) {
    float col=(angle/6.28318530718+0.5)*84.0;
    float column=floor(col);
    float drift=0.45+matrix_hash(float2(column,1.0))*0.85;
    float row=(height*0.5+0.5)*46.0;
    float scroll=row+t*drift*6.0+matrix_hash(float2(column,2.0))*32.0;
    float glyphRow=floor(scroll);
    float2 p=float2(frac(col),frac(scroll));
    float seed=matrix_hash(float2(column,glyphRow));
    // Pseudo-glyphs built from vertical, horizontal and diagonal strokes.
    float glyph=matrix_segment(p,float2(0.25,0.18),float2(0.25,0.82),0.065);
    glyph+=matrix_segment(p,float2(0.25,seed>0.5?0.25:0.72),float2(0.75,seed>0.5?0.25:0.72),0.063);
    glyph+=matrix_segment(p,float2(0.75,0.18),float2(seed>0.32?0.75:0.45,0.82),0.06);
    glyph=saturate(glyph);
    float active=step(0.29,matrix_hash(float2(column,glyphRow+11.0)));
    float trail=frac(t*drift*0.105+matrix_hash(float2(column,3.0)));
    float falling=frac(scroll/46.0);
    float tail=exp(-5.0*frac(falling-trail+1.0));
    float bright=pow(saturate(tail),9.0);
    float luminous=glyph*active*(0.12+0.65*tail);
    return float3(0.006,0.048,0.016)+luminous*float3(0.15,2.8,0.52)
        +glyph*active*bright*float3(0.70,1.25,0.55);
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
    } else if(mode < 2.5) {
        // Galaxy: a luminous Milky Way-like band plus sparse analytic stars.
        float band=exp(-14.0*abs(d.y+0.24*sin(angle*1.4+0.4)));
        float spiral=0.5+0.5*sin(angle*8.0+d.y*28.0-time*0.025);
        float cell=sin(angle*171.0)*sin(d.y*217.0);
        float stars=pow(saturate(cell),50.0);
        c=float3(0.025,0.035,0.13);
        c+=band*lerp(float3(0.35,0.19,0.85),float3(0.70,0.52,1.0),spiral)*0.95;
        c+=stars*float3(0.85,0.95,1.0)*0.95;
    } else if(mode < 3.5) {
        // Aquatic: underwater turquoise gradient, shallow light shafts, drifting caustics.
        float depth=saturate(0.5-0.5*d.y);
        float caustic=sin(angle*15.0+time*0.16+sin(d.y*12.0))
                     *sin(d.y*19.0-angle*8.0-time*0.11);
        float shaft=pow(saturate(sin(angle*6.0+time*0.04)),12.0)*saturate(d.y+0.4);
        c=lerp(float3(0.03,0.32,0.48),float3(0.08,0.78,0.88),horizon);
        c+=float3(0.30,0.76,0.78)*saturate(caustic)*0.48;
        c+=float3(0.42,0.85,0.78)*shaft*0.54;
        c*=1.0-depth*0.25;
    } else if(mode < 4.5) {
        // Rainforest: layered canopy silhouettes, warm forest shafts and distant haze.
        float canopy=0.28+0.16*sin(angle*5.0)+0.08*sin(angle*13.0+0.5);
        float leaves=sin(angle*29.0+time*0.025)*sin(d.y*37.0-time*0.017);
        float tree=exp(-85.0*abs(sin(angle*5.0+0.25)))*saturate(0.8-d.y);
        float mist=exp(-11.0*abs(d.y+0.1));
        float sun=pow(saturate(sin(angle*7.0+time*0.02)),20.0)*saturate(d.y+0.1);
        float foliage=1.0-smoothstep(canopy-0.06,canopy+0.06,d.y);
        c=lerp(float3(0.16,0.46,0.21),float3(0.54,0.76,0.38),horizon);
        c=lerp(c,float3(0.045,0.23,0.075)*(0.84+0.16*leaves),foliage*0.65);
        c=lerp(c,float3(0.03,0.13,0.04),tree*0.67);
        c+=float3(0.23,0.27,0.13)*mist*0.17+float3(0.45,0.40,0.20)*sun*0.17;
    } else {
        c=matrix_rain(angle,d.y,time);
    }
    // Palette boosts compensate for compositing a transparent sky over passthrough.
    // Preserve the global opacity control and avoid extra geometry or loops.
    return float4(saturate(c),saturate(strength));
}
