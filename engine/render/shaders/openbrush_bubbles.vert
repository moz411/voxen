#version 450
layout(location=0) in vec3 inPosition;
layout(location=1) in vec3 inCenter;
layout(location=2) in vec4 inColor;
layout(location=3) in vec4 inUV;
layout(push_constant) uniform BubbleCamera {
    mat4 viewProjection;
    vec4 cameraRight;
    vec4 cameraUp;
    vec4 settings;
} camera;
layout(location=0) out vec4 vColor;
layout(location=1) out vec2 vDisk;
layout(location=2) out float vPhase;
void main() {
    float t=camera.settings.x;
    float phase=inUV.z;
    float radius=inUV.w;
    vec3 offset=vec3(
        sin(t*0.8+phase)*0.045,
        sin(t*0.35+phase)*0.07+mod(t*0.012+phase*0.01,0.7),
        cos(t*0.65+phase)*0.035);
    vec3 right=camera.cameraRight.xyz;
    vec3 up=camera.cameraUp.xyz;
    vec3 pos=inCenter+offset+radius*(right*inUV.x+up*inUV.y);
    gl_Position=camera.viewProjection*vec4(pos,1.0);
    vColor=inColor;
    vDisk=inUV.xy;
    vPhase=phase+t*0.7;
}
