#version 450
// RUBIK CUBE — adapted from user-provided animated Rubik shader.
// Standalone analytical cubie ray intersections, no iChannel0 state.
// An animated quarter-turn cycles across X, Y and Z slices;
// this is visual animation, not a persistent Rubik puzzle solver.
layout(location=0) in vec2 uv;
layout(location=0) out vec4 outColor;
layout(push_constant) uniform PushConstants {mat4 mvp;vec4 parameters;} pc;
const float PI=3.14159265359;
mat2 rot(float a){float c=cos(a),s=sin(a);return mat2(c,-s,s,c);}
vec3 spin(vec3 p,int axis,float a){
 if(axis==0)p.yz=rot(a)*p.yz;
 else if(axis==1)p.xz=rot(a)*p.xz;
 else p.xy=rot(a)*p.xy;
 return p;
}
vec3 faceColor(vec3 normal,vec3 cubie){
 vec3 col=vec3(.023,.027,.033);
 // local +/- X Y Z faces; different material for each sticker.
 if(abs(normal.x)>.5 && sign(normal.x)*cubie.x>.5)
  col=normal.x>0.?vec3(.04,.77,.18):vec3(.06,.35,1.);
 if(abs(normal.y)>.5 && sign(normal.y)*cubie.y>.5)
  col=normal.y>0.?vec3(.98,.98,.93):vec3(1.,.88,.03);
 if(abs(normal.z)>.5 && sign(normal.z)*cubie.z>.5)
  col=normal.z>0.?vec3(.96,.08,.055):vec3(1.,.37,.025);
 return col;
}
bool boxHit(vec3 ro,vec3 rd,vec3 c,vec3 halfsize,out float t,out vec3 normal,out vec3 local){
 vec3 a=(c-halfsize-ro)/rd,b=(c+halfsize-ro)/rd;
 vec3 mn=min(a,b),mx=max(a,b);
 float nearT=max(max(mn.x,mn.y),mn.z),farT=min(min(mx.x,mx.y),mx.z);
 if(farT<max(nearT,0.))return false;
 t=nearT>0.?nearT:farT;
 vec3 q=ro+rd*t-c;
 vec3 n=abs(q/halfsize);
 normal=vec3(0.);
 if(n.x>=n.y && n.x>=n.z)normal.x=sign(q.x);
 else if(n.y>=n.z)normal.y=sign(q.y);
 else normal.z=sign(q.z);
 local=q;
 return true;
}
void main(){
 float time=pc.parameters.x*pc.parameters.y;
 // Consistent with other standalone Voxen effects: rendering to a quad.
 vec3 ro=vec3(0.,0.,4.8);
 vec3 rd=normalize(vec3(uv*1.20,-1.8));
 float orbit=time*.20;
 ro.xz=rot(orbit)*ro.xz;
 ro.yz=rot(.22*sin(time*.31))*ro.yz;
 rd.xz=rot(orbit)*rd.xz;
 rd.yz=rot(.22*sin(time*.31))*rd.yz;
 float best=1e9;
 vec3 color=vec3(0.);
 vec3 bestNormal=vec3(0.);
 // The turn eases toward a quarter turn, then resets to its original
 // arrangement as no persistent orientation state buffer is available.
 int axis=int(mod(floor(time*.55),3.));
 float phase=fract(time*.55);
 float turn=(phase<.20?0.:phase>.8?1.:smoothstep(.20,.80,phase))*1.57079632679;
 for(int xi=-1;xi<=1;xi++)
 for(int yi=-1;yi<=1;yi++)
 for(int zi=-1;zi<=1;zi++){
  vec3 cubie=vec3(float(xi),float(yi),float(zi));
  float layer=(axis==0?cubie.x:axis==1?cubie.y:cubie.z);
  float a=layer>.5?turn:0.;
  vec3 center=spin(cubie,axis,a);
  vec3 localRo=spin(ro,axis,-a)-cubie;
  vec3 localRd=spin(rd,axis,-a);
  float t;vec3 n,q;
  if(!boxHit(localRo,localRd,vec3(0.),vec3(.485),t,n,q) || t>=best)continue;
  vec3 sticker=faceColor(n,cubie);
  float edge=max(max(abs(q.x),abs(q.y)),abs(q.z));
  float inner=max(max(abs(q.x-n.x*.485),abs(q.y-n.y*.485)),abs(q.z-n.z*.485));
  // Distinguish stickers from the black plastic borders.
  if(inner>.41)sticker=vec3(.015,.018,.025);
  best=t;
  bestNormal=spin(n,axis,a);
  color=sticker;
 }
 if(best>1e8)discard;
 vec3 L=normalize(vec3(.8,1.1,1.6));
 float diffuse=max(dot(bestNormal,L),0.);
 float rim=pow(1.-max(dot(bestNormal,-rd),0.),3.);
 vec3 shaded=color*(.27+.78*diffuse)+vec3(.15)*rim*.30;
 shaded=pow(clamp(shaded,0.,1.),vec3(.82));
 outColor=vec4(shaded,pc.parameters.w);
}
