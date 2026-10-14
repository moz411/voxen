#version 450
// Instance records: world position.xyz + size, hue + shader ID.
layout(location=0) in vec4 instancePositionSize;
layout(location=1) in vec4 instanceStyle;
layout(push_constant) uniform Push {
    mat4 mvp; // per-eye projection * view for instanced rendering
    vec4 parameters; // x: time, y: animation speed
} pc;
layout(location=0) out vec3 localColor;
layout(location=1) out vec2 style;
const vec3 positions[8]=vec3[](
    vec3(-.5,-.5,-.5),vec3(.5,-.5,-.5),vec3(.5,.5,-.5),vec3(-.5,.5,-.5),
    vec3(-.5,-.5,.5),vec3(.5,-.5,.5),vec3(.5,.5,.5),vec3(-.5,.5,.5));
const int indices[36]=int[](
    0,2,1,0,3,2,4,5,6,4,6,7,0,1,5,0,5,4,3,7,6,3,6,2,0,4,7,0,7,3,1,2,6,1,6,5);
void main(){
    vec3 p=positions[indices[gl_VertexIndex]];
    gl_Position=pc.mvp*vec4(instancePositionSize.xyz+p*instancePositionSize.w,1.);
    localColor=p+vec3(.5);
    style=instanceStyle.xy;
}
