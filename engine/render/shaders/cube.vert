#version 450

layout(push_constant) uniform PushConstants {
    mat4 mvp;
} pc;

layout(location = 0) out vec3 color;

const vec3 positions[8] = vec3[](
    vec3(-0.5, -0.5, -0.5), vec3( 0.5, -0.5, -0.5),
    vec3( 0.5,  0.5, -0.5), vec3(-0.5,  0.5, -0.5),
    vec3(-0.5, -0.5,  0.5), vec3( 0.5, -0.5,  0.5),
    vec3( 0.5,  0.5,  0.5), vec3(-0.5,  0.5,  0.5)
);

const int indices[36] = int[](
    0,2,1, 0,3,2, 4,5,6, 4,6,7,
    0,1,5, 0,5,4, 3,7,6, 3,6,2,
    0,4,7, 0,7,3, 1,2,6, 1,6,5
);

void main() {
    vec3 p = positions[indices[gl_VertexIndex]];
    gl_Position = pc.mvp * vec4(p, 1.0);
    color = p + vec3(0.5);
}
