#version 450
// Volume smoke-test using the ORIGINAL cube.vert demo vertex shader.
// Its 3D position/color varying is known to have worked in the headset.
layout(location=0) in vec3 color;
layout(location=0) out vec4 outColor;
void main(){
    outColor=vec4(color,1.0);
}
