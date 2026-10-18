#version 450
// Artistic Mandelbulb shading in the vertex stage. Fragment input/output
// interface stays unchanged so the proven cyan path remains available.
layout(location=0) in vec3 position;
layout(location=1) in vec3 normal;
layout(location=0) out vec3 litColor;
layout(push_constant) uniform PushConstants {mat4 mvp;vec4 parameters;} pc;
// parameters = elapsed time, per-object speed, hue, density
vec3 palette(float phase){
    return 0.5+0.5*cos(6.28318530718*(phase+vec3(0.0,0.333333,0.666667)));
}
void main(){
    gl_Position=pc.mvp*vec4(position,1.0);
    vec3 n=normalize(normal);
    vec3 key=normalize(vec3(-0.48,0.78,0.43));
    vec3 fill=normalize(vec3(0.60,-0.18,-0.77));
    float diffuse=max(dot(n,key),0.0);
    float bounce=max(dot(n,fill),0.0);
    float slow=pc.parameters.x*pc.parameters.y*0.15;
    float modulation=0.045*sin(slow+position.y*17.0);
    vec3 base=palette(pc.parameters.z+modulation);
    // Gentle metallic-inspired contrast, no frame-dependent geometry.
    float light=0.20+0.68*diffuse+0.12*bounce;
    float sheen=pow(max(dot(n,normalize(key+vec3(0.0,0.0,1.0))),0.0),16.0);
    litColor=clamp(base*light+(0.10+0.14*pc.parameters.w)*sheen,0.0,1.0);
}
