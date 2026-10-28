#version 450
// Synthwave Sunset: adapted from user-supplied "another synthwave sunset thing".
// Audio sampling disabled; bounded raymarch suitable for mobile Vulkan.
layout(location=0) in vec2 uv;
layout(location=0) out vec4 outColor;
layout(push_constant) uniform PushConstants {mat4 mvp;vec4 parameters;} pc;
float jt;
float hash21(vec2 p){return fract(sin(dot(p,vec2(1.9898,7.233)))*45758.5433);}
float amp(vec2 p){return smoothstep(1.,8.,abs(p.x));}
float hashTerrain(vec2 p){
 float a=amp(p);
 float w=1.-.4*pow(clamp(.51+.49*sin((.02*(p.y+.5*p.x)-jt)*2.),0.,1.),64.);
 return a*pow(hash21(p),1.5)*w;
}
vec2 trinoise(vec2 p){
 p.x*=sqrt(1.5);p.y-=.5*p.x;
 vec2 d=fract(p);p-=d;
 bool c=d.x+d.y>1.;
 vec2 da=c?1.-d:d,db=c?d:1.-d;
 float n0=hashTerrain(p+float(c));
 float n1=hashTerrain(p+vec2(1,0));
 float n2=hashTerrain(p+vec2(0,1));
 float dx=clamp(da.x/max(db.y,.0001),0.,1.);
 float nn=mix(mix(n0,c?n1:n2,da.y),mix(n1,n2,d.y),dx);
 float edge=min(min((1.-dx)*db.y,da.x),da.y);
 return vec2(nn,edge);
}
vec2 sceneMap(vec3 p){vec2 n=trinoise(p.xz);return vec2(p.y-2.*n.x,n.y);}
vec3 normalAt(vec3 p){
 float e=.008;
 return normalize(vec3(sceneMap(p+vec3(e,0,0)).x-sceneMap(p-vec3(e,0,0)).x,
 sceneMap(p+vec3(0,e,0)).x-sceneMap(p-vec3(0,e,0)).x,
 sceneMap(p+vec3(0,0,e)).x-sceneMap(p-vec3(0,0,e)).x));
}
vec2 trace(vec3 ro,vec3 rd){
 float t=0.;
 for(int i=0;i<64;i++){
  vec3 p=ro+t*rd;vec2 s=sceneMap(p);
  if(abs(s.x)<max(.012,.003*t))return vec2(t,s.y);
  t+=clamp(abs(s.x)*.65,.025,2.0);
  if(t>110.||p.y>3.)break;
 }
 return vec2(-1.);
}
vec3 sky(vec3 r,vec3 ld){
 float haze=exp2(-5.*(abs(r.y)-.2*dot(r,ld)));
 vec3 col=clamp(mix(vec3(.4,.1,.7),vec3(.7,.1,.4),haze),0.,1.);
 float sun=1.-smoothstep(.2,.21,distance(r,ld));
 return mix(col,vec3(.75,.6,.3),sun);
}
void main(){
 vec2 p=uv;
 jt=mod(pc.parameters.x*pc.parameters.y,4000.);
 vec3 ro=vec3(0.,1.,-20000.+jt*10.);
 vec3 rd=normalize(vec3(p,1.33333));
 vec3 ld=normalize(vec3(0.,.125+.05*sin(.1*jt),1.));
 vec2 hit=trace(ro,rd);
 vec3 col=sky(rd,ld);
 if(hit.x>0.){
  vec3 pos=ro+hit.x*rd;
  vec3 n=normalAt(pos);
  float diff=dot(n,ld)+.1*n.y;
  vec3 lit=vec3(.1,.11,.18)*diff;
  vec3 reflected=sky(reflect(rd,n),ld);
  lit=mix(lit,reflected,.05+.95*pow(max(1.+dot(rd,n),0.),5.));
  lit=mix(lit,vec3(.8,.1,.92),1.-smoothstep(0.,.05,hit.y));
  vec3 fog=exp2(-hit.x*vec3(.14,.1,.28));
  col=mix(col,lit,fog);
 }
 outColor=vec4(clamp(col,0.,1.),pc.parameters.w);
}
