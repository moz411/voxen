#version 450
layout(location=0) out vec2 clipXY;
// Fullscreen triangle; no vertex buffer needed.
void main() {
    vec2 p=vec2((gl_VertexIndex<<1)&2,gl_VertexIndex&2);
    clipXY=p*2.0-1.0;
    gl_Position=vec4(clipXY,0.9999,1.0);
}
