#version 450
layout(location=0) out vec3 localColor;
void main() {
    vec2 p=vec2((gl_VertexIndex<<1)&2,gl_VertexIndex&2);
    gl_Position=vec4(p*2.0-1.0,0.99999,1.0);
    localColor=vec3(p,0.5);
}
