#version 450
// Cheap diagnostic fill for tracked Meta Scene wall rectangles.
// Push constants: parameters = vec4(red,green,blue,alpha).
layout(location=0) in vec2 uv;
layout(location=0) out vec4 outColor;
layout(push_constant) uniform PushConstants { mat4 mvp; vec4 parameters; } pc;
void main() {
    outColor=vec4(clamp(pc.parameters.rgb,0.0,1.0),clamp(pc.parameters.a,0.0,1.0));
}
