#version 450
layout(push_constant) uniform Push {mat4 mvp;vec4 settings;} pc;
layout(location=0) out vec2 uv;
const vec2 corners[6]=vec2[6](vec2(-1,-1),vec2(1,-1),vec2(1,1),vec2(-1,-1),vec2(1,1),vec2(-1,1));
void main(){vec2 p=corners[gl_VertexIndex];uv=p*0.5+0.5;gl_Position=pc.mvp*vec4(p,0.0,1.0);}
