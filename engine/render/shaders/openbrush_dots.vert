#version 450
// One GPU instance per native Open Brush Dots control point.
// Six procedural vertices per quad; no static vertex buffer required.
layout(location=0) in vec3 inCenter;
layout(location=1) in vec4 inColor;
layout(location=2) in float inDiameter;
layout(push_constant) uniform DotsCamera {
    mat4 viewProjection;
    vec4 cameraRight;
    vec4 cameraUp;
} camera;
layout(location=0) out vec2 vUV;
layout(location=1) out vec4 vColor;
void main() {
    const vec2 corners[6] = vec2[6](
        vec2(-1.0,-1.0), vec2(1.0,-1.0), vec2(1.0,1.0),
        vec2(-1.0,-1.0), vec2(1.0,1.0), vec2(-1.0,1.0)
    );
    vec2 corner = corners[gl_VertexIndex];
    vec3 offset = (camera.cameraRight.xyz * corner.x +
                   camera.cameraUp.xyz * corner.y) * (0.5 * inDiameter);
    gl_Position = camera.viewProjection * vec4(inCenter + offset, 1.0);
    vUV = corner * 0.5 + 0.5;
    vColor = inColor;
}
