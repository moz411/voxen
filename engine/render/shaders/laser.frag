#version 450
layout(location = 0) in vec3 localColor;
layout(location = 0) out vec4 outColor;
void main() {
    outColor = vec4(0.22, 0.95, 0.95, 1.0);
}
