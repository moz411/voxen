#version 450
layout(location=0) in vec3 vNormal;
layout(location=1) in vec2 vUV;
layout(set=0,binding=0) uniform sampler2D baseColorMap;
layout(location=0) out vec4 outColor;
layout(push_constant) uniform PushConstants {mat4 mvp;vec4 parameters;} pc;
void main(){
    vec3 n=normalize(vNormal);
    float diffuse=max(dot(n,normalize(vec3(-0.48,0.78,0.43))),0.0);
    vec4 sampleColor=texture(baseColorMap,vUV);
    vec3 lighting=vec3(0.26+0.74*diffuse);
    outColor=vec4(pc.parameters.rgb*sampleColor.rgb*lighting,
                  pc.parameters.a*sampleColor.a);
}
