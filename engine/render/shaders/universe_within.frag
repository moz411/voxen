#version 450
// The Universe Within — Martijn Steinrucken (BigWings), 2018.
// CC BY-NC-SA 3.0. Adapted from user-provided Shadertoy GLSL.
// Original: https://youtu.be/GAhu4ngQa48
// Mouse parallax and iChannel0 FFT omitted; use autonomous animation.
layout(location=0) in vec2 uv;
layout(location=0) out vec4 outColor;
layout(push_constant) uniform PushConstants {mat4 mvp;vec4 parameters;} pc;
float S(float a,float b,float t){return smoothstep(a,b,t);}
float N21(vec2 p){
 vec3 a=fract(vec3(p.x,p.y,p.x)*vec3(213.897,653.453,253.098));
 a+=dot(a,a.yzx+79.76);return fract((a.x+a.y)*a.z);
}
vec2 getPos(vec2 id,vec2 offs,float t){
 float n=N21(id+offs),n1=fract(n*10.),n2=fract(n*100.);
 float a=t+n;return offs+vec2(sin(a*n1),cos(a*n2))*.4;
}
float segmentDist(vec2 a,vec2 b,vec2 p){
 vec2 pa=p-a,ba=b-a;
 float d=max(dot(ba,ba),.00001);
 return length(pa-ba*clamp(dot(pa,ba)/d,0.,1.));
}
float lineGlow(vec2 a,vec2 b,vec2 p){
 float len=length(a-b),d=segmentDist(a,b,p);
 float fade=S(1.5,.5,len)+S(.05,.02,abs(len-.75));
 return S(.04,.01,d)*fade;
}
float netLayer(vec2 st,float n,float t){
 vec2 id=floor(st)+n;st=fract(st)-.5;
 vec2 points[9];
 for(int y=-1;y<=1;y++)
  for(int x=-1;x<=1;x++)
   points[(y+1)*3+x+1]=getPos(id,vec2(x,y),t);
 float m=0.,sparkle=0.;
 for(int i=0;i<9;i++){
  m+=lineGlow(points[4],points[i],st);
  float d=max(length(st-points[i]),.015);
  float s=.005/(d*d)*S(1.,.7,d);
  float pulse=sin((fract(points[i].x)+fract(points[i].y)+t)*5.)*.4+.6;
  sparkle+=s*pow(pulse,20.);
 }
 m+=lineGlow(points[1],points[3],st)+lineGlow(points[1],points[5],st);
 m+=lineGlow(points[7],points[5],st)+lineGlow(points[7],points[3],st);
 float phase=(sin(t+n)+sin(t*.1))*.25+.5;
 phase+=pow(sin(t*.1)*.5+.5,50.)*5.;
 return m+sparkle*phase;
}
void main(){
 vec2 p=uv*.7;
 float time=pc.parameters.x*pc.parameters.y;
 float t=time*.1,s=sin(t),c=cos(t);
 mat2 rot=mat2(c,s,-s,c);
 vec2 st=rot*p;
 // Subtle autonomous parallax replaces mouse motion.
 vec2 parallax=rot*(vec2(sin(t*.7),cos(t*.53))*.12);
 float m=0.;
 for(int i=0;i<4;i++){
  float f=float(i)*.25;
  float z=fract(t+f);
  float size=mix(15.,1.,z);
  float fade=S(0.,.6,z)*S(1.,.8,z);
  m+=fade*netLayer(st*size-parallax*z,f,time);
 }
 vec3 base=vec3(s,cos(t*.4),-sin(t*.24))*.4+.6;
 vec3 color=max(base*m,vec3(0.));
 color*=max(0.,1.-dot(p,p));
 color*=S(0.,20.,mod(time,230.))*S(224.,200.,mod(time,230.));
 color=1.-exp(-color*.65);
 outColor=vec4(clamp(color,0.,1.),pc.parameters.w);
}
