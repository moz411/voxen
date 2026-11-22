#version 450
layout(location=0) in vec4 vColor;
layout(location=1) in vec3 vShape;
layout(location=0) out vec4 outColor;
void main() {
    float edge=abs(vShape.x);
    float core=1.0-smoothstep(0.05,0.37,edge);
    float glow=1.0-smoothstep(0.15,1.0,edge);
    float spark=0.76+0.24*sin(vShape.y*47.0-vShape.z);
    float alpha=vColor.a*(0.58*glow+0.42*core)*spark*(1.0-smoothstep(0.78,1.0,vShape.y));
    vec3 color=mix(vColor.rgb,vec3(0.92,0.97,1.0),0.84*core);
    outColor=vec4(color,clamp(alpha,0.0,0.95));
}
