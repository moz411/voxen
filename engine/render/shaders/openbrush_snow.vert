#version 450
layout(location=0) in vec3 inPosition;
layout(location=1) in vec3 inCenter;
layout(location=2) in vec4 inColor;
layout(location=3) in vec4 inUV;
layout(push_constant) uniform SnowCamera {
    mat4 viewProjection;
    vec4 cameraRight;
    vec4 cameraUp;
    vec4 settings;
} camera;
layout(location=0) out vec4 vColor;
layout(location=1) out vec2 vDisk;
void main() {
    float phase=inUV.x;
    float t=camera.settings.x;
    float period=3.0+mod(phase,2.0);
    float fall=mod(t*0.22+phase*0.159,period);
    vec3 offset=vec3(
        sin(t*0.9+phase)*0.045,
        -fall*0.18,
        cos(t*0.7+phase)*0.04);
    vec2 local=inUV.yz;
    float spin=t*0.65+phase;
    mat2 rotation=mat2(cos(spin),sin(spin),-sin(spin),cos(spin));
    vec2 rotated=rotation*local;
    float radius=inUV.w;
    vec3 billboard=(camera.cameraRight.xyz*rotated.x+camera.cameraUp.xyz*rotated.y)*radius;
    gl_Position=camera.viewProjection*vec4(inCenter+offset+billboard,1.0);
    vColor=inColor;
    vDisk=local;
}
