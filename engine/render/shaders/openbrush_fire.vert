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
void main() {
    float t=camera.settings.x;
    float tip=inUV.x;
    float phase=inUV.y;
    float radius=inUV.z;
    vec3 position=inPosition;
    float sway=tip*tip*radius;
    float turbulence=sin(t*7.3+phase*2.7+tip*13.0)*sin(t*3.1+phase+tip*8.0);
    position.x+=sway*(0.85*sin(t*4.7+phase)+0.35*sin(t*9.2+phase*1.8)+0.24*turbulence);
    position.z+=sway*(0.7*cos(t*3.9+phase)+0.2*turbulence);
    position.y+=tip*radius*0.22*sin(t*6.3+phase);
    float flicker=0.85+0.15*sin(t*9.0+phase);
    vColor=vec4(inColor.rgb*flicker,inColor.a*flicker);
    gl_Position=camera.viewProjection*vec4(position,1.0);
}
