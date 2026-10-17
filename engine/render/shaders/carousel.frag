#version 450
layout(location=0) in vec2 uv;
layout(location=0) out vec4 outColor;
layout(push_constant) uniform PushConstants { mat4 mvp; vec4 parameters; } pc;
float box2(vec2 p,vec2 center,vec2 halfsize) {
 vec2 d=abs(p-center)-halfsize;
 return 1.0-smoothstep(0.0,0.012,max(d.x,d.y));
}
void main() {
 vec2 p=uv;
 // Phosphor-green pixel-frame with 8 page tabs and a central content preview.
 float panel=box2(p,vec2(0.0),vec2(0.96,0.96));
 float inner=box2(p,vec2(0.0),vec2(0.91,0.89));
 float frame=max(panel-inner,0.0);
 float top=box2(p,vec2(0.0,0.71),vec2(0.85,0.015));
 float tabs=0.0;
 int selectedPage=clamp(int(pc.parameters.w+0.5),0,7);
 for(int i=0;i<8;i++) {
   float x=-0.74+float(i)*0.21;
   float fill=box2(p,vec2(x,0.80),vec2(0.066,0.048));
   tabs=max(tabs,fill*(i==selectedPage?1.0:0.22));
 }
 float line=box2(p,vec2(0.0,0.20),vec2(0.68,0.012));
 float line2=box2(p,vec2(0.0,-0.12),vec2(0.54,0.012));
 float line3=box2(p,vec2(0.0,-0.43),vec2(0.37,0.012));
 float pulse=0.65+0.35*sin(pc.parameters.x*2.0);
 float intensity=max(max(frame,top),max(tabs,max(line,max(line2,line3))*0.6*pulse));
 outColor=vec4(vec3(0.06,0.95,0.35)*intensity, max(panel*0.20,intensity*0.92));
}
