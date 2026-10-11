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
    } else if (mode > 4.5 && mode < 5.5) { // ice
        float crack=pow(abs(sin((p.x+p.z)*32+sin(p.y*19+t))),12);
        c=float3(0.2,0.68,1.0)*(0.7+0.3*crack);
    } else if (mode > 5.5 && mode < 6.5) { // aurora: layered colored curtains
        float wave = sin(p.x*12.0 + t*0.8 + sin(p.z*8.0-t*0.35)*1.8);
        float bands = 0.5 + 0.5*sin(p.y*21.0 + wave*3.0 + t*0.6);
        c = lerp(float3(0.03,0.12,0.35), float3(0.1,1.0,0.58), bands);
    } else if (mode > 6.5 && mode < 7.5) { // pulse: traveling concentric rings
        float radius = length(p);
        float ring = 1.0-smoothstep(0.08,0.25,abs(sin(radius*37.0-t*5.0)));
        c = c*(0.2+0.8*ring) + float3(0.06,0.2,0.4)*ring;
    } else if (mode > 7.5 && mode < 8.5) { // circuit: stable orthogonal trails
        float3 q = abs(frac(p*13.0+0.5)-0.5);
        float wires = 1.0-smoothstep(0.018,0.07,min(q.x,min(q.y,q.z)));
        float flicker = 0.72+0.28*sin(t*3.0+p.x*17.0+p.y*13.0);
        c = c*0.12 + float3(0.1,0.95,0.65)*wires*flicker;
    } else if (mode > 8.5 && mode < 9.5) { // marble: warped isosurfaces
        float warp = sin(p.x*13.0+p.y*7.0+t*0.3)*sin(p.z*11.0-t*0.2);
        float veins = 0.5+0.5*sin((p.x+p.y+p.z)*24.0+warp*6.0);
        c = lerp(c*0.15+float3(0.05,0.06,0.13),c*0.85+float3(0.25,0.2,0.35),veins);
    } else if (mode > 9.5 && mode < 10.5) { // nebula: low-frequency moving clouds
        float n = sin(p.x*9.0+t*0.4)*sin(p.y*12.0-t*0.3);
        n += 0.5*sin(p.z*19.0+p.x*7.0+t*0.7);
        float glow = saturate(0.5+0.38*n);
        c = lerp(float3(0.08,0.01,0.2),float3(0.85,0.18,0.9),glow);
    } else if (mode > 10.5 && mode < 11.5) { // glitch: quantized time slices
        float row = floor(p.y*38.0);
        float tick = floor(t*8.0);
        float shift = frac(sin(row*127.1+tick*311.7)*43758.5453);
        float stripe = step(0.78,shift);
        float channel = sin((p.x+stripe*0.22)*28.0+t*2.0);
        c = lerp(c,float3(0.9,0.18,0.7),stripe*0.7) *
            (0.55+0.45*abs(channel));
    }
    float light = 0.55 + 0.45*saturate(dot(normalize(input.normal),normalize(float3(0.35,0.8,-0.4))));
    return float4(saturate(c*light),input.tint.a);
}
