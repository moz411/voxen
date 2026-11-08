#version 450
// Smoke: straight-alpha texture blending, no depth writes. The Open Brush
// source uses v_color * tint * MainTex for .tilt input.
layout(location=0) in vec2 vUV;
layout(location=1) in vec4 vColor;
layout(set=0,binding=0) uniform sampler2D smokeMainTex;
layout(location=0) out vec4 outColor;
void main() {
    vec4 color = vColor * texture(smokeMainTex,vUV);
    if(color.a < 0.003) discard;
    outColor = color;
}
