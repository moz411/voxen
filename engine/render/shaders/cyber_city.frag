#version 450
// CyberCity Quest adaptation — inspired by source shared by user.
// Original: Patrice TERRIER (www.objreader.com), CyberCity Step 3t.
// This is a reduced standalone interpretation, NOT a full port of
// Common + Buffer A font atlas + Image. No external textures or audio.
layout(location=0) in vec2 uv;
layout(location=0) out vec4 outColor;
layout(push_constant) uniform PushConstants {mat4 mvp;vec4 parameters;} pc;
float hash21(vec2 p){vec3 q=fract(vec3(p.x,p.y,p.x)*.1031);q+=dot(q,q.yzx+33.33);return fract((q.x+q.y)*q.z);}
float boxSdf(vec3 p,vec3 b){vec3 q=abs(p)-b;return length(max(q,0.))+min(max(q.x,max(q.y,q.z)),0.);}
float height(vec2 id){float a=hash21(id),b=hash21(id+31.7);return 1.5+3.5*a*a+(b>.85?3.+6.*hash21(id+57.3):0.);}
float mapCity(vec3 p,out vec2 cell){
 cell=floor(p.xz);
 float ground=p.y;
 float nearest=1e3;
 for(int z=-1;z<=1;z++)for(int x=-1;x<=1;x++){
  vec2 id=cell+vec2(x,z);
  if(mod(id.x,3.)<.5||mod(id.y,6.)<.5)continue;
  float h=height(id);
  vec2 seed=vec2(hash21(id+1.7),hash21(id+8.4));
  vec2 halfSize=vec2(.34,.38)+.09*seed;
  vec3 center=vec3(id.x+.5,h*.5,id.y+.5);
  float d=boxSdf(p-center,vec3(halfSize.x,h*.5,halfSize.y));
  if(h>4.5)d=min(d,boxSdf(p-vec3(center.x,h*.74,center.z),vec3(halfSize*.65,h*.26)));
  nearest=min(nearest,d);
 }
 return min(ground,nearest);
}
vec3 normalAt(vec3 p){
 vec2 id;
 float e=.015;
 return normalize(vec3(mapCity(p+vec3(e,0,0),id)-mapCity(p-vec3(e,0,0),id),
 mapCity(p+vec3(0,e,0),id)-mapCity(p-vec3(0,e,0),id),
 mapCity(p+vec3(0,0,e),id)-mapCity(p-vec3(0,0,e),id)));
}
vec3 facade(vec3 p,vec3 n,float t){
 vec2 id=floor(p.xz);
 if(abs(n.y)>.5)return vec3(.035,.045,.07);
 vec2 grid=vec2(abs(n.x)>.5?p.z:p.x,p.y)/.085;
 vec2 index=floor(grid),f=fract(grid);
 float cellHash=hash21(index+id*11.3);
 float lit=step(.53,cellHash);
 float aperture=step(.2,f.x)*step(f.x,.82)*step(.18,f.y)*step(f.y,.8);
 vec3 amber=vec3(1.,.68,.37),cyan=vec3(.12,.85,1.);
 vec3 light=mix(amber,cyan,step(.75,hash21(id+7.3)));
 float flicker=.75+.25*sin(t*.5+cellHash*15.);
 return vec3(.015,.025,.05)+aperture*lit*flicker*light;
}
void main(){
 float t=pc.parameters.x*pc.parameters.y;
 vec2 p=uv;
 // Moving camera down a procedural street; stylized neon-noir mood.
 vec3 ro=vec3(1.5,2.0,t*.32+2.5);
 float angle=.08*sin(t*.075);
 vec3 forward=normalize(vec3(sin(angle),-.09,cos(angle)));
 vec3 right=normalize(cross(vec3(0,1,0),forward));
 vec3 up=normalize(cross(forward,right));
 vec3 rd=normalize(forward*1.3+right*p.x+up*p.y);
 vec3 col=mix(vec3(.012,.01,.035),vec3(.12,.055,.18),clamp(rd.y+.25,0.,1.));
 float dist=0.,hit=-1.;
 vec2 id;
 for(int i=0;i<56;i++){
  vec3 pos=ro+rd*dist;
  float d=mapCity(pos,id);
  if(d<.015){hit=dist;break;}
  dist+=clamp(d,.02,1.5);
  if(dist>65.)break;
 }
 if(hit>0.){
  vec3 pos=ro+rd*hit,n=normalAt(pos);
  col=facade(pos,n,t);
  if(n.y>.7){
   vec2 grid=fract(pos.xz*8.);
   float wet=.04+.14*pow(max(dot(reflect(-rd,n),normalize(vec3(.2,.6,.8))),0.),6.);
   col=vec3(.012,.013,.026)+wet*vec3(.15,.42,.55);
   col+=.025*step(.92,grid.x);
  }
  float fog=exp(-hit*.045);
  col=mix(vec3(.07,.045,.11),col,fog);
 }
 // Projected neon ribbons, aerial glow and slanted rain.
 vec2 lights=vec2(sin(p.x*20.+t*.2),cos(p.y*13.-t*.1));
 col+=vec3(.16,.03,.28)*.04*pow(max(lights.x*lights.y,0.),6.);
 float rain=hash21(floor(vec2(p.x*180.+p.y*40.,p.y*110.-t*14.)));
 col+=vec3(.18,.24,.33)*step(.987,rain)*.17;
 col=clamp(col,0.,10.);
 col=(col*(2.51*col+.03))/(col*(2.43*col+.59)+.14);
 col*=1.-.25*dot(p,p);
 outColor=vec4(clamp(col,0.,1.),pc.parameters.w);
}
