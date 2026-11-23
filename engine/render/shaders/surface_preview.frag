#version 450
layout(location=0) in vec2 uv;
layout(location=0) out vec4 outColor;
layout(push_constant) uniform PushConstants { mat4 mvp; vec4 parameters; } pc;
void main() {
    vec2 a=abs(uv);
    float edge=1.0-smoothstep(0.955,0.985,max(a.x,a.y));
    float border=1.0-edge;
    float cross=(1.0-smoothstep(0.012,0.024,min(a.x,a.y)))*
                (1.0-smoothstep(0.12,0.25,length(uv)));
    float circle=1.0-smoothstep(0.012,0.027,abs(length(uv)-0.15));
    float mask=max(border,max(cross,circle));
    vec3 color=mix(vec3(1.0,0.67,0.17),vec3(0.18,1.0,0.82),pc.parameters.x);
    outColor=vec4(color,mask*(0.75+0.2*sin(pc.parameters.y*5.0)));
}
