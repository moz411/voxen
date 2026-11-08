#version 450
// Open Brush Dots prototype: instanced camera-facing quad.
// Vertex buffer: unit quad corners in [-1,1], UV in [0,1].
// Instance data: world-space center, RGBA, diameter (meters).
layout(location=0) in vec2 inCorner;
layout(location=1) in vec2 inUV;
layout(location=2) in vec3 inCenter;
layout(location=3) in vec4 inColor;
layout(location=4) in float inDiameter;
layout(set=0,binding=0) uniform Camera {
    mat4 viewProjection;
    vec4 cameraRight;
    vec4 cameraUp;
} camera;
layout(location=0) out vec2 vUV;
layout(location=1) out vec4 vColor;
void main() {
    vec3 offset = (camera.cameraRight.xyz * inCorner.x +
                   camera.cameraUp.xyz * inCorner.y) * (0.5 * inDiameter);
    gl_Position = camera.viewProjection * vec4(inCenter + offset, 1.0);
    vUV = inUV;
    vColor = inColor;
}
