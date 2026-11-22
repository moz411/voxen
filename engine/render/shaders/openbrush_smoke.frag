#version 450
// Smoke: straight-alpha texture blending, no depth writes. The Open Brush
// source uses v_color * tint * MainTex for .tilt input.
layout(location=0) in vec2 vUV;
layout(location=1) in vec4 vColor;
layout(location=2) in float vSeed;
layout(set=0,binding=0) uniform sampler2D smokeMainTex;
layout(location=0) out vec4 outColor;
void main() {
    // Keep the original OpenBrush Smoke atlas as the silhouette; add only a
    // subtle procedural density breakup (two cheap, derivative-free waves).
    vec2 centered=vUV*2.0-1.0;
    float radius=dot(centered,centered);
    vec4 color = vColor * texture(smokeMainTex,vUV);
    float wisps=0.78+0.15*sin(centered.x*11.0+centered.y*6.0+vSeed*13.0)
                    +0.07*sin(centered.y*19.0-centered.x*8.0+vSeed*31.0);
    color.a*=clamp(wisps,0.52,1.0)*(1.0-smoothstep(0.68,1.5,radius));
    if(color.a < 0.003) discard;
    outColor = color;
}
