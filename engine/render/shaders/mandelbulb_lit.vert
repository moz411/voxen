#version 450
layout(location=0) in vec3 position;
layout(location=1) in vec3 normal;
layout(location=0) out vec3 litColor;
layout(push_constant) uniform PushConstants {mat4 mvp;vec4 parameters;} pc;
void main(){
    gl_Position=pc.mvp*vec4(position,1.0);
    vec3 n=normalize(normal);
    vec3 light=normalize(vec3(-0.4,0.75,0.6));
    float diffuse=max(dot(n,light),0.0);
    float rim=pow(1.0-abs(n.z),3.0);
    vec3 base=vec3(0.10,0.85,0.90);
    litColor=base*(0.28+0.72*diffuse)+0.14*rim*vec3(0.4,0.8,1.0);
}
