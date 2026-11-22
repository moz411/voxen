#version 450
// Analytic flame silhouette, layered turbulence and blackbody-inspired gradient.
// No textures, loops or derivative-dependent operations; tuned for mobile Vulkan.
layout(location=0) in vec4 vColor;
layout(location=1) in vec3 vFlame; // height, signed ribbon coordinate, phase
layout(location=2) in vec2 vEmberDisk;
layout(location=0) out vec4 outColor;
float hash21(vec2 p) {
    return fract(sin(dot(p,vec2(127.1,311.7)))*43758.5453);
}
float noise(vec2 p) {
    vec2 i=floor(p), f=fract(p);
    f=f*f*(3.0-2.0*f);
    return mix(mix(hash21(i),hash21(i+vec2(1,0)),f.x),
               mix(hash21(i+vec2(0,1)),hash21(i+vec2(1,1)),f.x),f.y);
}
void main() {
    if(vFlame.x<0.0) {
        // Ember quads: circular, soft luminous core instead of rectangular cards.
        float r2=dot(vEmberDisk,vEmberDisk);
        float glow=exp(-4.5*r2);
        outColor=vec4(mix(vColor.rgb,vec3(1.0,0.95,0.64),exp(-18.0*r2)*0.65),vColor.a*glow);
        return;
    }
    float h=clamp(vFlame.x,0.0,1.0);
    float side=abs(vFlame.y);
    float phase=vFlame.z;
    // Two decorrelated bands break up the straight ribbon edges.
    float n=noise(vec2(h*7.5+phase*0.17,phase*0.31));
    float n2=noise(vec2(h*16.0-phase*0.23,phase*0.57));
    float distort=(n-0.5)*0.20+(n2-0.5)*0.09;
    float edge=1.0-smoothstep(0.62+distort,1.02+distort,side);
    float tip=1.0-smoothstep(0.70+0.12*(n-0.5),1.0,h);
    float core=pow(max(0.0,1.0-side),1.5);
    float flicker=0.78+0.22*n2;
    // White-hot base -> gold -> orange -> deep red at the thinning tip.
    vec3 hot=vec3(1.0,0.98,0.73);
    vec3 gold=vec3(1.0,0.64,0.12);
    vec3 red=vec3(0.83,0.11,0.025);
    vec3 palette=mix(hot,gold,smoothstep(0.03,0.48,h+side*0.13));
    palette=mix(palette,red,smoothstep(0.45,0.95,h+side*0.10));
    palette=mix(palette,hot,0.50*core*(1.0-h));
    float alpha=vColor.a*edge*tip*flicker*(0.55+0.45*core);
    outColor=vec4(palette,clamp(alpha,0.0,0.95));
}
