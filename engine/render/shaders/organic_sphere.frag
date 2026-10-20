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
    int material=clamp(int(floor(pc.parameters.z+0.0001)),0,3);
    vec3 base=palette(fract(phase));
    float bands=0.5+0.5*sin(22.0*vPulse+6.0*vPosition.y-pc.parameters.x*pc.parameters.y*2.0);
    vec3 color=base*(0.22+0.68*diffuse)+vec3(0.05,0.30,0.44)*bands*0.32;
    color+=mix(vec3(0.08,0.55,0.8),vec3(0.65,0.22,0.95),bands)*rim*0.48;
    if(material==1){ // EMBER
        float lava=pow(0.5+0.5*sin(14.0*vPulse+5.0*vPosition.y-2.0*pc.parameters.x),3.0);
        color=mix(vec3(0.12,0.012,0.006),vec3(1.0,0.22,0.018),lava);
        color+=vec3(0.4,0.10,0.0)*rim;
    }else if(material==2){ // ABYSS
        float veins=pow(0.5+0.5*sin(22.0*vPulse+9.0*vPosition.x+pc.parameters.x),7.0);
        color=vec3(0.015,0.025,0.085)*(0.3+diffuse)+
              vec3(0.02,0.7,0.95)*(0.65*veins+0.7*rim);
    }else if(material==3){ // MATRIX
        float columns=step(0.83,fract(vPosition.x*23.0+vPosition.z*7.0));
        float drops=pow(0.5+0.5*sin(vPosition.y*35.0+pc.parameters.x*5.0+
                  floor(vPosition.x*23.0)*2.1),8.0);
        color=vec3(0.005,0.035,0.009)+vec3(0.08,0.95,0.19)*
              (columns*drops+0.18*rim);
    }
    outColor=vec4(clamp(color,0.0,1.0),1.0);
}
