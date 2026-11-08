#version 450
layout(location=0) in vec3 inPosition;
layout(location=1) in vec3 inNormal;
layout(location=2) in vec4 inColor;
// VXMSH002 preserves UV0.xyzw; the generic fallback currently uses only color.
layout(location=3) in vec4 inUV;
layout(push_constant) uniform Camera { mat4 viewProjection; } camera;
layout(location=0) out vec4 vColor;
void main() {
    gl_Position=camera.viewProjection*vec4(inPosition,1.0);
    vColor=inColor;
}
