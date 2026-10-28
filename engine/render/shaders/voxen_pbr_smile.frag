#version 450
// PBR SMILE — original shader by jorge2017a3 (27 October 2026).
// Ported to a transparent standalone Vulkan Voxen preview.
// Simplified geometry/lighting for Quest 3; inspired by the provided
// SDF sculpt, GGX + simulated warm SSS + ACES tone mapping.
layout(location=0) in vec2 uv;
layout(location=0) out vec4 outColor;
layout(push_constant) uniform PushConstants {mat4 mvp;vec4 parameters;} pc;
const float PI=3.14159265359;
struct Material {vec3 albedo;float roughness;float metallic;float sss;};
mat2 rot(float a){float c=cos(a),s=sin(a);return mat2(c,s,-s,c);}
float sdEllipsoid(vec3 p,vec3 r){float k0=length(p/r),k1=length(p/(r*r));return k0*(k0-1.)/max(k1,.0001);}
float sdSphere(vec3 p,float r){return length(p)-r;}
float sdCylinder(vec3 p,float r,float h){vec2 q=abs(vec2(length(p.xz),p.y))-vec2(r,h);return min(max(q.x,q.y),0.)+length(max(q,0.));}
vec2 opUnion(vec2 a,vec2 b){return a.x<b.x?a:b;}
vec2 opSubtract(vec2 a,vec2 b){return -b.x>a.x?vec2(-b.x,b.y):a;}
vec2 map(vec3 p,float time){
 // Local body coordinates are centered rather than at original (10,0,0).
 // Rotate slightly from side to side, revealing the sculpted eye sockets.
 p.xz=rot(.12*sin(time*2.))*p.xz;
 vec2 head=vec2(sdSphere(p,1.),3.);
 vec2 eyesocket1=vec2(sdEllipsoid(p-vec3(-.35,.25,.84),vec3(.22,.30,.35)),4.);
 vec2 eyesocket2=vec2(sdEllipsoid(p-vec3(.35,.25,.84),vec3(.22,.30,.35)),4.);
 head=opSubtract(head,eyesocket1);
 head=opSubtract(head,eyesocket2);
 vec2 eye1=vec2(sdEllipsoid(p-vec3(-.35,.24,.82),vec3(.17,.25,.28)),9.);
 vec2 eye2=vec2(sdEllipsoid(p-vec3(.35,.24,.82),vec3(.17,.25,.28)),9.);
 head=opUnion(head,eye1);
 head=opUnion(head,eye2);
 // Deep smile shaped like a wide rounded cavity with an inner tooth band.
 vec3 m=p-vec3(0.,-.40,.84);
 float mouth=sdEllipsoid(m,vec3(.55,.23,.28));
 head=opSubtract(head,vec2(mouth,10.));
 float teeth=sdEllipsoid(m-vec3(0.,.115,-.08),vec3(.48,.065,.17));
 teeth=max(teeth,-mouth-.01);
 head=opUnion(head,vec2(teeth,11.));
 return head;
}
vec3 normalAt(vec3 p,float time){
 float e=.0025;
 return normalize(vec3(map(p+vec3(e,0,0),time).x-map(p-vec3(e,0,0),time).x,
 map(p+vec3(0,e,0),time).x-map(p-vec3(0,e,0),time).x,
 map(p+vec3(0,0,e),time).x-map(p-vec3(0,0,e),time).x));
}
Material material(float id){
 Material m;
 if(id==3.){m.albedo=vec3(1.,.65,.05);m.roughness=.25;m.metallic=0.;m.sss=.8;}
 else if(id==9.){m.albedo=vec3(.012);m.roughness=.5;m.metallic=.21;m.sss=.2;}
 else if(id==10.){m.albedo=vec3(.025,.005,.005);m.roughness=.5;m.metallic=0.;m.sss=0.;}
 else if(id==11.){m.albedo=vec3(.95,.96,.98);m.roughness=.15;m.metallic=0.;m.sss=.1;}
 else {m.albedo=vec3(.9,.5,.05);m.roughness=.3;m.metallic=0.;m.sss=.5;}
 return m;
}
float ggx(vec3 N,vec3 H,float rough){
 float a=rough*rough, a2=a*a, nh=max(dot(N,H),0.);
 float denominator=nh*nh*(a2-1.)+1.;
 return a2/max(PI*denominator*denominator,.0001);
}
float geoSchlick(float nv,float rough){
 float r=rough+1.,k=r*r/8.;
 return nv/max(nv*(1.-k)+k,.0001);
}
vec3 fresnelSchlick(float cosTheta,vec3 f0){return f0+(1.-f0)*pow(clamp(1.-cosTheta,0.,1.),5.);}
vec3 evalLight(vec3 N,vec3 V,vec3 L,Material m,vec3 intensity){
 vec3 H=normalize(V+L),F0=mix(vec3(.04),m.albedo,m.metallic);
 vec3 F=fresnelSchlick(max(dot(H,V),0.),F0);
 float g=geoSchlick(max(dot(N,V),0.),m.roughness)*geoSchlick(max(dot(N,L),0.),m.roughness);
 vec3 spec=ggx(N,H,m.roughness)*g*F/max(4.*max(dot(N,V),0.)*max(dot(N,L),0.),.0001);
 float sss=pow(clamp(dot(-V,L+N*.4),0.,1.),2.)*m.sss;
 vec3 diffuse=mix(m.albedo/PI,vec3(1.,.2,.05),sss*.4);
 return ((1.-F)*(1.-m.metallic)*diffuse+spec)*intensity*max(dot(N,L),0.);
}
vec3 aces(vec3 x){return clamp(x*(2.51*x+.03)/(x*(2.43*x+.59)+.14),0.,1.);}
void main(){
 float time=pc.parameters.x*pc.parameters.y;
 vec2 p=uv*1.6;
 vec3 ro=vec3(0.,0.,3.6),rd=normalize(vec3(p,-2.4));
 float t=0.;vec2 hit=vec2(100.,0.);bool found=false;
 for(int i=0;i<64;i++){
   vec3 pos=ro+t*rd;
   hit=map(pos,time);
   if(hit.x<.003){found=true;break;}
   t+=clamp(hit.x*.85,.003,.18);
   if(t>6.)break;
 }
 if(!found)discard;
 vec3 pos=ro+t*rd,N=normalAt(pos,time),V=-rd;
 Material m=material(hit.y);
 // Four studio lights, as in the original.
 vec3 col=evalLight(N,V,normalize(vec3(-1.,1.,1.)),m,vec3(2.7,2.4,2.1));
 col+=evalLight(N,V,normalize(vec3(1.,.2,1.)),m,vec3(1.,.7,.4));
 col+=evalLight(N,V,normalize(vec3(0.,1.,-1.)),m,vec3(1.3,1.4,1.7));
 col+=evalLight(N,V,normalize(vec3(-.3,-1.,1.)),m,vec3(.65,.45,.25));
 col+=mix(vec3(.30,.15,.05),vec3(.10,.15,.25),N.y*.5+.5)*m.albedo;
 col=pow(aces(max(col,vec3(0.))),vec3(1./2.2));
 outColor=vec4(col,pc.parameters.w);
}
