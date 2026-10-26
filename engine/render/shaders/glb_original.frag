#version 450
layout(location=0) in vec3 vNormal;
layout(location=0) out vec4 outColor;
layout(push_constant) uniform PushConstants { mat4 mvp; vec4 parameters; } pc;
void main() {
    vec3 n=normalize(vNormal);
    vec3 key=normalize(vec3(-0.48,0.78,0.43));
    vec3 fill=normalize(vec3(0.60,-0.18,-0.77));
    float light=0.24+0.64*max(dot(n,key),0.0)+0.12*max(dot(n,fill),0.0);
    // The base-color factor is in linear color space in glTF.
    // This first pass does not sample baseColorTexture or perform full PBR.
    outColor=vec4(clamp(pc.parameters.rgb*light,0.0,1.0),pc.parameters.a);
}
