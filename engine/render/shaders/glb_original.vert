#version 450
layout(location=0) in vec3 position;
layout(location=1) in vec3 normal;
layout(location=3) in vec4 color0;
layout(location=0) out vec3 vNormal;
layout(location=1) out vec4 vColor;
layout(push_constant) uniform PushConstants { mat4 mvp; vec4 parameters; } pc;
void main() {
    gl_Position = pc.mvp * vec4(position,1.0);
    vNormal = normalize(normal);
    vColor = color0;
}
