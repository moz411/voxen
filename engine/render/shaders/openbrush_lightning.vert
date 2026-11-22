#version 450
layout(location=0) in vec3 inPosition;
layout(location=1) in vec3 inCenter;
layout(location=2) in vec4 inColor;
layout(location=3) in vec4 inUV;
layout(push_constant) uniform Camera {
    mat4 viewProjection;
    vec4 right;
    vec4 up;
    vec4 settings;
} camera;
layout(location=0) out vec4 vColor;
layout(location=1) out vec3 vShape;
void main() {
    float time=camera.settings.x;
    float progress=inUV.x,phase=inUV.z;
    float jitter=sin(progress*39.0+phase*7.3+time*20.0)*0.45+
                 sin(progress*83.0-phase*3.7-time*27.0)*0.24;
    float taper=1.0-progress*0.72;
    vec3 center=inCenter+(camera.right.xyz*0.68+camera.up.xyz*0.32)*
        jitter*inUV.w*5.0*progress;
    gl_Position=camera.viewProjection*vec4(center+camera.right.xyz*inUV.y*inUV.w*taper,1.0);
    vColor=inColor;
    vShape=vec3(inUV.y,progress,phase+time*8.0);
}
