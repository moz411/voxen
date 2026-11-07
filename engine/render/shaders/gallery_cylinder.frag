#version 450
layout(location=0) in vec2 uv;
layout(location=1) in vec3 normal;
layout(location=0) out vec4 outColor;
layout(push_constant) uniform PushConstants { mat4 mvp; vec4 parameters; } pc;
void main() {
    float stripes=0.5+0.5*cos(6.2831853*(uv.x*3.0+pc.parameters.x*0.035));
    vec3 base=0.45+0.45*cos(6.2831853*(uv.x/3.0+vec3(0.0,0.33,0.67)+pc.parameters.z*0.08));
    float rim=pow(1.0-abs(normal.z),2.0);
    float edge=1.0-smoothstep(0.0,0.05,min(uv.y,1.0-uv.y));
    vec3 col=base*(0.55+0.45*stripes)+vec3(0.15,0.55,0.8)*edge*0.8+rim*0.08;
    outColor=vec4(col*pc.parameters.y,0.94);
}
