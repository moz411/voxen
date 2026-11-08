#version 450
// Experimental Quest 3 Smoke billboard renderer.
// Uses Open Brush's UV0.z rotation and a_normal particle center.
// Motion is an inexpensive curl-like approximation, not the original simplex curl.
layout(location=0) in vec3 inPosition;
layout(location=1) in vec3 inCenter;
layout(location=2) in vec4 inColor;
layout(location=3) in vec4 inUV;
layout(push_constant) uniform SmokeCamera {
    mat4 viewProjection;
    vec4 cameraRight;
    vec4 cameraUp;
    vec4 settings;
} camera;
layout(location=0) out vec2 vUV;
layout(location=1) out vec4 vColor;
void main() {
    // Particle mesh corners carry UV (0,0)-(1,1). Rotate each camera-facing
    // quad around its center; do not use gl_VertexIndex, which is globally based.
    vec2 corner = vec2(inUV.x < .5 ? -1.0 : 1.0,
                       inUV.y < .5 ? 1.0 : -1.0);
    float rotation = inUV.z;
    mat2 rot = mat2(cos(rotation), sin(rotation),
                    -sin(rotation), cos(rotation));
    vec2 rotated = rot * corner;
    float halfSize = length(inPosition - inCenter) * 0.70710678118;
    vec3 offset = (camera.cameraRight.xyz * rotated.x +
                   camera.cameraUp.xyz * rotated.y) * halfSize;

    float t = camera.settings.x * .32;
    vec3 p = inCenter * 2.4;
    // Divergence-free trigonometric flow: curl of a periodic vector potential.
    vec3 flow = vec3(
        -sin(p.z+t) - cos(p.y+t),
        -sin(p.x+t) - cos(p.z+t),
        -sin(p.y+t) - cos(p.x+t));
    vec3 displaced = inCenter + flow * 0.035;
    gl_Position = camera.viewProjection * vec4(displaced + offset, 1.0);
    vUV = inUV.xy;
    vColor = inColor;
}
