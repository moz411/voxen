#version 450
layout(location=0) in vec2 uv;
layout(location=0) out vec4 outColor;
layout(set=0,binding=0) uniform sampler2D picture;
layout(push_constant) uniform PushConstants { mat4 mvp; vec4 parameters; } pc;
void main() {
    // Exact Meta plane quad: normalized local XY maps the complete image to
    // the complete detected surface, with no tiling or spherical distortion.
    vec2 texcoord=vec2(uv.x*0.5+0.5,0.5-uv.y*0.5);
    outColor=vec4(texture(picture,texcoord).rgb,1.0);
}
