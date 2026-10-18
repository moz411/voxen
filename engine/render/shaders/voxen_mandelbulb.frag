#version 450
// Step 2: use the exact proven cube.vert varying at location 0.
// No fractal math, discard, ray calculation, or push-constant reads.
layout(location = 0) in vec3 localColor;
layout(location = 0) out vec4 outColor;
void main() {
    outColor = vec4(localColor, 1.0);
}
