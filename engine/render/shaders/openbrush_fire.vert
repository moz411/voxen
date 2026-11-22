#version 450
layout(location=0) in vec3 inPosition;
layout(location=1) in vec3 inCenter;
layout(location=2) in vec4 inColor;
layout(location=3) in vec4 inUV;
layout(push_constant) uniform FireCamera {
    mat4 viewProjection;
    vec4 cameraRight;
    vec4 cameraUp;
    vec4 settings;
} camera;
layout(location=0) out vec4 vColor;
float hash11(float n) {return fract(sin(n)*43758.5453123);}
float noise2(vec2 p) {
    vec2 i=floor(p), f=fract(p);
    f=f*f*(3.0-2.0*f);
    float a=hash11(dot(i,vec2(127.1,311.7)));
    float b=hash11(dot(i+vec2(1,0),vec2(127.1,311.7)));
    float c=hash11(dot(i+vec2(0,1),vec2(127.1,311.7)));
    float d=hash11(dot(i+vec2(1,1),vec2(127.1,311.7)));
    return mix(mix(a,b,f.x),mix(c,d,f.x),f.y);
}
void main() {
    float t=camera.settings.x;
    float tip=inUV.x;
    float phase=inUV.y;
    vec3 position=inPosition;
    if(tip<0.0) {
        float lifetime=1.7+fract(phase*0.137)*1.2;
        float age=mod(t*0.8+phase*0.37,lifetime)/lifetime;
        vec3 center=inCenter+vec3(sin(t*2.3+phase)*0.06,age*0.42,cos(t*1.8+phase)*0.05);
        position=center+camera.cameraRight.xyz*inUV.z*0.008+
                        camera.cameraUp.xyz*inUV.w*0.008;
        vColor=vec4(inColor.rgb,inColor.a*(1.0-age)*(1.0-age));
    } else {
        float radius=inUV.z;
        float n=noise2(vec2(tip*5.0+phase,t*2.5));
        float n2=noise2(vec2(tip*11.0-phase,t*4.8));
        float sway=tip*tip*radius;
        position.x+=sway*(sin(t*4.7+phase)+1.3*(n-0.5)+0.5*(n2-0.5));
        position.z+=sway*(cos(t*3.9+phase)+1.1*(n2-0.5));
        position.y+=tip*radius*0.20*sin(t*6.3+phase);
        float flicker=0.78+0.22*n2;
        vColor=vec4(inColor.rgb*flicker,inColor.a*flicker);
    }
    gl_Position=camera.viewProjection*vec4(position,1.0);
}
