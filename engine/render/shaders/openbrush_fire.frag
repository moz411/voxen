#version 450
// Flame-specific emissive response, preserves premixed core/tongue vertex colors.
layout(location=0) in vec4 vColor;
layout(location=1) in vec2 vFlame;
layout(location=0) out vec4 outColor;
void main() {
    float tip=max(vFlame.x,0.0);
    float ember=1.0-step(0.0,vFlame.x);
    float fade=1.0-smoothstep(0.74,1.0,tip);
    float glow=0.7+0.3*(1.0-tip);
    vec3 hot=mix(vColor.rgb,vec3(1.0,0.91,0.55),0.25*(1.0-tip));
    outColor=vec4(hot*glow,vColor.a*mix(fade,1.0,ember));
}
