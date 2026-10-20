#version 450
layout(location=0) in vec3 vNormal;
layout(location=1) in vec3 vPosition;
layout(location=2) in float vPulse;
layout(location=0) out vec4 outColor;
layout(push_constant) uniform PushConstants {
    mat4 mvp;
    vec4 parameters; // time, speed, hue, amplitude
} pc;
vec3 palette(float t){
    return 0.5+0.5*cos(6.2831853*(t+vec3(0.0,0.33,0.67)));
}
void main(){
    vec3 n=normalize(vNormal);
    vec3 light=normalize(vec3(-0.55,0.78,0.5));
    float diffuse=max(dot(n,light),0.0);
    float rim=pow(1.0-abs(n.z),3.0);
    float phase=pc.parameters.z+0.11*vPulse+0.045*pc.parameters.x*pc.parameters.y;
    vec3 base=palette(phase);
    float bands=0.5+0.5*sin(22.0*vPulse+6.0*vPosition.y-pc.parameters.x*pc.parameters.y*2.0);
    vec3 color=base*(0.22+0.68*diffuse)+vec3(0.05,0.30,0.44)*bands*0.32;
    color+=mix(vec3(0.08,0.55,0.8),vec3(0.65,0.22,0.95),bands)*rim*0.48;
    outColor=vec4(clamp(color,0.0,1.0),1.0);
}
