#version 450
layout(location=0) out vec4 outColor;
layout(push_constant) uniform PushConstants { mat4 mvp; vec4 parameters; } pc;
void main() { outColor=vec4(0.05,1.0,0.38,0.78); }
