#version 450

layout(push_constant) uniform PushConstants {
    mat4 mvp;
    vec4 parameters; // time, speed, scale, shape (0 sphere, 1 torus)
} pc;

layout(location = 0) out vec3 localColor;

// Six vertices per grid cell; UV topology generated entirely on the GPU.
const int SEGMENTS = 48;
const int RINGS = 24;
const float PI = 3.141592653589793;

void main() {
    int cell = gl_VertexIndex / 6;
    int corner = gl_VertexIndex % 6;
    int x = cell % SEGMENTS;
    int y = cell / SEGMENTS;
    const ivec2 offsets[6] = ivec2[](
        ivec2(0,0), ivec2(1,0), ivec2(1,1),
        ivec2(0,0), ivec2(1,1), ivec2(0,1)
    );
    vec2 uv = vec2(x + offsets[corner].x, y + offsets[corner].y) /
              vec2(SEGMENTS, RINGS);
    float a = uv.x * 2.0 * PI;
    float b = uv.y * PI;
    float time = pc.parameters.x * pc.parameters.y;
    float shape = pc.parameters.w;
    vec3 p;
    if (shape < 0.5) {
        p = vec3(sin(b) * cos(a), cos(b), sin(b) * sin(a));
    } else {
        float u = uv.y * 2.0 * PI;
        p = vec3((0.76 + 0.28 * cos(u)) * cos(a),
                 0.28 * sin(u),
                 (0.76 + 0.28 * cos(u)) * sin(a));
    }
    float wave = sin(9.0 * p.x + time * 1.4) *
                 cos(7.0 * p.y - time * 1.1) *
                 sin(8.0 * p.z + time * 0.7);
    p *= 1.0 + wave * 0.16;
    localColor = p * 0.5 + 0.5;
    gl_Position = pc.mvp * vec4(p, 1.0);
}
