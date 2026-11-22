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
    position.x+=tip*sin(t*4.7+phase*1.3)*radius*0.9;
    position.z+=tip*cos(t*3.9+phase)*radius*0.65;
    position.y=inCenter.y+(position.y-inCenter.y)*
        (0.8+0.2*sin(t*6.3+phase));
    float flicker=0.78+0.22*sin(t*9.0+phase);
    vColor=vec4(inColor.rgb*flicker,inColor.a*flicker);
    gl_Position=camera.viewProjection*vec4(position,1.0);
}
