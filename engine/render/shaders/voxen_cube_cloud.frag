#version 450
// PULSING CUBE CLOUD — adapted from user-provided Shadertoy GLSL.
// Procedural cube lattice with animated sizes, rotation and transparent MR background.
layout(location=0) in vec2 uv;
layout(location=0) out vec4 outColor;
layout(push_constant) uniform PushConstants {mat4 mvp;vec4 parameters;} pc;
mat2 rot(float a){float c=cos(a),s=sin(a);return mat2(c,-s,s,c);}
float cube(vec3 p){return max(max(abs(p.x),abs(p.y)),abs(p.z));}
float scene(vec3 p,float t){
 p.xz=rot(-.3*sin(.3*t))*p.xz;
 p.yz=rot(.3+.18*cos(.3*t))*p.yz;
 const float n=7.;
 vec3 q=p*n;
 vec3 id=floor(q+.5);
 vec3 local=fract(q+.5)-.5;
 float sz=.20-.20*sin(2.*length(id)+2.*t);
 float cell=cube(local)-sz;
 float bound=cube(q)-n*3.5;
 return max(cell,bound)/n;
}
vec3 normalAt(vec3 p,float t){
 float e=.004;
 return normalize(vec3(scene(p+vec3(e,0,0),t)-scene(p-vec3(e,0,0),t),
 scene(p+vec3(0,e,0),t)-scene(p-vec3(0,e,0),t),
 scene(p+vec3(0,0,e),t)-scene(p-vec3(0,0,e),t)));
}
void main(){
 float t=pc.parameters.x*pc.parameters.y;
 vec3 ro=vec3(0.,0.,2.2);
 vec3 rd=normalize(vec3(uv,-1.7));
 float d=0.;bool hit=false;vec3 pos=ro;
 for(int i=0;i<84;i++){
  pos=ro+rd*d;
  float h=scene(pos,t);
  if(h<.002){hit=true;break;}
  d+=clamp(h*.8,.003,.12);
  if(d>3.6)break;
 }
 if(!hit)discard;
 vec3 n=normalAt(pos,t);
 float diffuse=.35+.65*max(dot(n,normalize(vec3(.5,.8,1.))),0.);
 float rim=pow(1.-max(dot(n,-rd),0.),3.);
 vec3 color=vec3(.6,.8,1.)*(diffuse+.32*rim);
 color*=exp(-d*.12);
 outColor=vec4(clamp(color,0.,1.),pc.parameters.w);
}
