#version 450
layout(location=0) in vec2 uv;
layout(location=1) in vec3 normal;
layout(location=0) out vec4 outColor;
layout(set=0,binding=0) uniform sampler2D picture;
layout(push_constant) uniform PushConstants { mat4 mvp; vec4 parameters; } pc;
void main() {
    // Repeat the chosen picture around the circumference three times.
    vec2 texcoord=vec2(fract(uv.x),1.0-uv.y);
    vec3 rgb=texture(picture,texcoord).rgb;
    float rim=1.0-smoothstep(0.0,0.035,min(uv.y,1.0-uv.y));
    outColor=vec4(rgb*pc.parameters.y+vec3(0.06,0.22,0.30)*rim,1.0);
}
