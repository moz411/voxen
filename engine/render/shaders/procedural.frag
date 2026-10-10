#version 450

// A procedural animated material: no textures or external assets.
layout(location = 0) in vec3 localColor;
layout(location = 0) out vec4 outColor;

layout(push_constant) uniform PushConstants {
    mat4 mvp;
    vec4 parameters; // x: elapsed seconds, y: animation speed, z: scale, w: unused
} pc;

void main() {
    vec3 p = localColor * 2.0 - 1.0;
    float t = pc.parameters.x * pc.parameters.y;
    float scale = pc.parameters.z;

    float a = sin(scale * p.x * 3.7 + t * 1.1);
    float b = sin(scale * p.y * 5.1 - t * 1.4);
    float c = sin(scale * p.z * 4.3 + t * 0.7);
    float energy = (a * b + c) * 0.5;

    vec3 cold = vec3(0.015, 0.18, 0.65);
    vec3 warm = vec3(0.94, 0.18, 0.64);
    vec3 color = mix(cold, warm, smoothstep(-0.75, 0.75, energy));
    float filaments = pow(1.0 - abs(sin(10.0 * energy + t * 0.8)), 14.0);
    color += filaments * vec3(0.50, 0.85, 1.0);
    outColor = vec4(color, 1.0);
}
