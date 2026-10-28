#version 450
// Ocean Waves — adapted from user-provided afl_ext MIT licensed GLSL.
// Meta Quest: reduced wave octaves and fixed raymarch budget.
layout(location=0) in vec2 uv;
layout(location=0) out vec4 outColor;
layout(push_constant) uniform PushConstants {mat4 mvp;vec4 parameters;} pc;
const float depth=1.0;
vec2 wavedx(vec2 pos,vec2 dir,float freq,float phase){
 float x=dot(dir,pos)*freq+phase;
 float w=exp(sin(x)-1.0);
 return vec2(w,-w*cos(x));
}
float waves(vec2 pos,int iterations,float time){
 float phase=length(pos)*0.1;
 float index=0.,freq=1.,tm=2.,weight=1.,sum=0.,total=0.;
 for(int i=0;i<20;i++){
  if(i>=iterations)break;
  vec2 dir=vec2(sin(index),cos(index));
  vec2 res=wavedx(pos,dir,freq,time*tm+phase);
  pos+=dir*res.y*weight*.38;
  sum+=res.x*weight;total+=weight;
  weight*=.8;freq*=1.18;tm*=1.07;index+=1232.399963;
 }
 return sum/max(total,.00001);
}
float heightAt(vec2 p,float time){return waves(p,10,time)*depth-depth;}
vec3 normalAt(vec2 p,float time){
 float e=.025,h=heightAt(p,time);
 return normalize(vec3(heightAt(p-vec2(e,0),time)-h,e,
                       heightAt(p-vec2(0,e),time)-h));
}
float intersectPlane(vec3 ro,vec3 rd,float y){
 return (y-ro.y)/min(rd.y,-.0001);
}
vec3 sunDirection(float t){return normalize(vec3(-.07735,.5+sin(t*.2+2.6)*.45,.57735));}
vec3 atmosphere(vec3 rd,vec3 sun){
 float trick=1.0/(rd.y+max(.1,-rd.y+.12));
 float trick2=1.0/(sun.y*11.+1.);
 float rays=pow(abs(dot(sun,rd)),2.);
 vec3 suncolor=mix(vec3(1.),max(vec3(0.),vec3(1.)-vec3(5.5,13.,22.4)/22.4),clamp(trick2,0.,1.));
 vec3 blue=vec3(5.5,13.,22.4)/22.4*suncolor;
 vec3 b=max(vec3(0.),blue-vec3(5.5,13.,22.4)*.002*(trick-6.*sun.y*sun.y));
 return max(vec3(0.),b*trick*(.24+rays*.24)*(1.+pow(1.-rd.y,3.)))*.5;
}
float sunGlow(vec3 rd,vec3 sd){return pow(max(0.,dot(rd,sd)),720.)*210.;}
vec3 tonemap(vec3 c){return pow(clamp((c*(2.51*c+.03))/(c*(2.43*c+.59)+.14),0.,1.),vec3(1./2.2));}
void main(){
 float t=pc.parameters.x*pc.parameters.y;
 vec3 ray=normalize(vec3(uv,1.5));
 vec3 sun=sunDirection(t);
 vec3 color;
 if(ray.y>=0.0){
  color=atmosphere(ray,sun)+vec3(sunGlow(ray,sun));
 }else{
  vec3 origin=vec3(t*.2,1.5,1.);
  float start=intersectPlane(origin,ray,0.);
  float end=intersectPlane(origin,ray,-depth);
  float dist=start;
  for(int i=0;i<24;i++){
   vec3 pos=origin+dist*ray;
   float delta=pos.y-heightAt(pos.xz,t);
   if(delta<max(.012,dist*.0005))break;
   dist+=clamp(delta/max(-ray.y,.1),.015,3.);
   if(dist>end) {dist=end;break;}
  }
  vec3 hit=origin+ray*dist;
  vec3 N=normalAt(hit.xz,t);
  N=normalize(mix(N,vec3(0.,1.,0.),.8*min(1.,sqrt(max(dist,0.)*.01)*1.1)));
  float fresnel=.04+.96*pow(1.-max(0.,dot(-N,ray)),5.);
  vec3 reflected=normalize(reflect(ray,N));reflected.y=abs(reflected.y);
  vec3 reflection=atmosphere(reflected,sun)+vec3(sunGlow(reflected,sun));
  vec3 scattering=vec3(.0293,.0698,.1717)*.1*(.2+clamp((hit.y+depth)/depth,0.,1.));
  color=fresnel*reflection+scattering;
 }
 outColor=vec4(tonemap(max(color,vec3(0.))*2.),pc.parameters.w);
}
