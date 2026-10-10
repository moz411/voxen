#version 450
layout(push_constant) uniform PushConstants {
    mat4 mvp;
    vec4 parameters;
} pc;
layout(location = 0) out vec2 uv;
// Camera-facing quad in local XY, spanning -1..1.
const vec2 corners[6] = vec2[](
    vec2(-1.0,-1.0), vec2(1.0,-1.0), vec2(1.0,1.0),
    vec2(-1.0,-1.0), vec2(1.0,1.0), vec2(-1.0,1.0)
);
void main() {
    uv = corners[gl_VertexIndex];
    gl_Position = pc.mvp * vec4(uv, 0.0, 1.0);
}
