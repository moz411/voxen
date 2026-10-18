#version 450
// Exact known-good laser.frag interface and constant output, built as
// voxenMandelbulbFrag. A/B isolates shader embedding from varying interpolation.
layout(location = 0) in vec3 localColor;
layout(location = 0) out vec4 outColor;
void main() {
    outColor = vec4(0.22, 0.95, 0.95, 1.0);
}
