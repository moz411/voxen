#version 450
// One animated 3D creature: central globe + eight segmented tentacles.
// Generated from gl_VertexIndex, no vertex buffers.
layout(push_constant) uniform Push {mat4 mvp;vec4 parameters;} pc;
layout(location=0) out vec3 vNormal;
layout(location=1) out vec3 vPosition;
layout(location=2) out float vPulse;
const float TAU=6.28318530718;
const int BODY_SEG=24;
const int BODY_RING=16;
const int BODY_VERTS=BODY_SEG*BODY_RING*6;
const int TENT_SEG=12;
const int TENT_SIDES=6;
const int VERTS_PER_TENT=TENT_SEG*TENT_SIDES*6;
vec3 spine(float u,float tent,float t){
 int mode=int(pc.parameters.w+0.5);
 float angle=TAU*(tent/8.0);
 if(mode==14)angle+=2.5*u+0.25*sin(t*0.7);
 float r=0.29+0.73*u;
 if(mode==13)r+=0.15*u*sin(t*2.1+tent*1.2);
 float sway=0.10*sin(t*1.4+u*8.0+tent*0.8)*u;
 if(mode==13)sway+=0.24*u*sin(t*3.0-u*9.0+tent);
 if(mode==14)sway+=0.16*u*sin(t*1.5+u*17.0+tent*0.6);
 return vec3(cos(angle)*r-sin(angle)*sway,0.12*sin(t+tent)-0.26*u+0.09*sin(u*11.-t*2.+tent)+(mode==13 ? 0.30*u*sin(t*2.6-u*6.0+tent) : mode==14 ? 0.14*u*cos(t+u*TAU*2.0) : 0.0),sin(angle)*r+cos(angle)*sway);
}
void main(){
 float t=pc.parameters.x*max(pc.parameters.y,0.01);
 vec3 pos;vec3 norm;
 if(gl_VertexIndex<BODY_VERTS){
  int cell=gl_VertexIndex/6;
  int corner=gl_VertexIndex%6;
  const ivec2 offsets[6]=ivec2[6](ivec2(0,0),ivec2(1,0),ivec2(1,1),ivec2(0,0),ivec2(1,1),ivec2(0,1));
  vec2 uv=vec2(cell%BODY_SEG,cell/BODY_SEG)+vec2(offsets[corner]);
  uv/=vec2(BODY_SEG,BODY_RING);
  float phi=uv.y*3.14159265359;
  float theta=uv.x*TAU;
  norm=vec3(sin(phi)*cos(theta),cos(phi),sin(phi)*sin(theta));
  float bodyPulse=int(pc.parameters.w+0.5)==13 ? 0.055*sin(t*3.5) : 0.015*sin(t*2.0);
  pos=norm*(0.34+bodyPulse);
 } else {
  int id=gl_VertexIndex-BODY_VERTS;
  int tent=id/VERTS_PER_TENT;
  int local=id%VERTS_PER_TENT;
  int cell=local/6;int corner=local%6;
  const ivec2 offsets[6]=ivec2[6](ivec2(0,0),ivec2(1,0),ivec2(1,1),ivec2(0,0),ivec2(1,1),ivec2(0,1));
  int axis=cell/TENT_SIDES;int side=cell%TENT_SIDES;
  vec2 uv=vec2(float(axis+offsets[corner].y)/float(TENT_SEG),
               float(side+offsets[corner].x)/float(TENT_SIDES));
  vec3 center=spine(uv.x,float(tent),t);
  vec3 tangent=normalize(spine(min(uv.x+0.01,1.0),float(tent),t)-spine(max(uv.x-0.01,0.0),float(tent),t));
  vec3 reference=abs(tangent.y)<0.9?vec3(0,1,0):vec3(1,0,0);
  vec3 sideA=normalize(cross(tangent,reference));
  vec3 sideB=normalize(cross(tangent,sideA));
  norm=sideA*cos(uv.y*TAU)+sideB*sin(uv.y*TAU);
  float radius=mix(0.067,0.009,uv.x);
  pos=center+norm*radius;
 }
 vNormal=norm;
 vPosition=pos;
 vPulse=float(gl_VertexIndex>=BODY_VERTS);
 gl_Position=pc.mvp*vec4(pos,1);
}
