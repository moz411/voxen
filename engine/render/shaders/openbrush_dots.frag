#version 450
// Radial falloff is a placeholder, NOT the original Open Brush Dots texture.
// Pipeline must enable premultiplied-alpha blending and disable depth writes.
layout(location=0) in vec2 vUV;
layout(location=1) in vec4 vColor;
layout(location=0) out vec4 outColor;
void main() {
    vec2 p = vUV * 2.0 - 1.0;
    float radius2 = dot(p, p);
    float coverage = 1.0 - smoothstep(0.65, 1.0, radius2);
    float alpha = clamp(vColor.a * coverage, 0.0, 1.0);
    outColor = vec4(vColor.rgb * alpha, alpha);
}
