#version 450
layout(location=0) in vec3 position;
layout(location=1) in vec3 normal;
layout(location=0) out vec3 vNormal;
layout(push_constant) uniform PushConstants { mat4 mvp; vec4 parameters; } pc;
void main() {
    gl_Position = pc.mvp * vec4(position,1.0);
    vNormal = normalize(normal);
}
