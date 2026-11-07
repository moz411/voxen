#version 450
layout(push_constant) uniform PushConstants { mat4 mvp; vec4 parameters; } pc;
layout(location=0) out vec2 uv;
layout(location=1) out vec3 normal;
const int SEG=48;
const float TAU=6.28318530718;
void main() {
    int cell=gl_VertexIndex/6, corner=gl_VertexIndex%6;
    int i=cell%SEG;
    const ivec2 offsets[6]=ivec2[6](
        ivec2(0,0),ivec2(1,0),ivec2(0,1),
        ivec2(1,0),ivec2(1,1),ivec2(0,1));
    int x=i+offsets[corner].x;
    float v=float(offsets[corner].y);
    float angle=TAU*float(x)/float(SEG);
    vec2 circle=vec2(cos(angle),sin(angle));
    normal=vec3(circle.x,0.0,circle.y);
    uv=vec2(3.0*float(x)/float(SEG),v);
    gl_Position=pc.mvp*vec4(circle.x,v*2.0-1.0,circle.y,1.0);
}
