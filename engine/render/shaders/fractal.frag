#version 450
layout(location = 0) in vec3 localColor;
layout(location = 0) out vec4 outColor;
layout(push_constant) uniform PushConstants {
    mat4 mvp;
    vec4 parameters;
} pc;

float sdSphere(vec3 p, float r) { return length(p) - r; }
float sdTorus(vec3 p, vec2 t) {
    vec2 q = vec2(length(p.xz) - t.x, p.y);
    return length(q) - t.y;
}
float field(vec3 p) {
    float t = pc.parameters.x * pc.parameters.y;
    p += 0.13 * sin(vec3(p.y * 8.0, p.z * 6.0, p.x * 7.0) + t);
    return min(sdSphere(p, 0.53), sdTorus(p.yzx, vec2(0.52, 0.13)));
}
vec3 normalAt(vec3 p) {
    float d = 0.002;
    return normalize(vec3(field(p + vec3(d,0,0)) - field(p - vec3(d,0,0)),
                          field(p + vec3(0,d,0)) - field(p - vec3(0,d,0)),
                          field(p + vec3(0,0,d)) - field(p - vec3(0,0,d))));
}
void main() {
    vec2 uv = localColor.xy * 2.0 - 1.0;
    vec3 ro = vec3(uv, 1.6);
    vec3 rd = vec3(0.0, 0.0, -1.0);
    float dist = 0.0;
    bool hit = false;
    vec3 p;
    for (int i=0; i<48; ++i) {
        p = ro + rd * dist;
        float d = field(p);
        if (d < 0.003) { hit = true; break; }
        dist += max(d, 0.008);
        if (dist > 3.0) break;
    }
    if (!hit) {
        outColor = vec4(0.015, 0.035, 0.10, 1.0);
        return;
    }
    vec3 n = normalAt(p);
    float diffuse = 0.2 + 0.8 * max(dot(n, normalize(vec3(-0.5,0.9,1.1))),0.0);
    vec3 color = mix(vec3(0.03,0.36,0.95),vec3(1.0,0.16,0.58),
                     0.5 + 0.5 * sin(p.y * 13.0 + pc.parameters.x));
    color *= diffuse;
    color += vec3(0.25,0.42,0.75) * pow(max(dot(n,vec3(0,0,1)),0.0),20.0);
    outColor = vec4(color, 1.0);
}
