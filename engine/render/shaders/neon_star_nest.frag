#version 450
// Neon Star Nest — Quest-bounded port of user-supplied neon tunnel + volumetric star nest.
layout(location=0) in vec2 uv;
layout(location=0) out vec4 outColor;
layout(push_constant) uniform PushConstants {mat4 mvp;vec4 parameters;} pc;
const float formuparam=.53;
const float tile=.85;
void main(){
 float time=pc.parameters.x*pc.parameters.y;
 vec2 q=uv;
 vec3 rd=normalize(vec3(q,1.-dot(q,q)*.25));
 vec3 c=vec3(0.0);
 float dist=0.;
 // Original 111 steps reduced to 48.
 for(int i=1;i<=48;i++){
  vec3 p=rd*dist;
  vec3 x=p+vec3(time*10.,0.,0.);
  float m=0.;
  for(int j=0;j<2;j++){
   float size=j==0?1.:24./54.;
   vec3 k=abs(fract(x/size)*size-size*.5);
   float v=min(max(k.x,k.y),min(max(k.y,k.z),max(k.x,k.z)))-size*.25;
   m=min(m,v);
  }
  p=x;
  float s=.21;
  for(int j=0;j<4;j++){
   float grid=10.1+2.1*cos(time);
   p.xy=floor(p.xy*grid)/grid;
   p.y-=time;p.x+=time*.05;
   float a=time*.02;
   p.xz=mat2(cos(a),sin(a),-sin(a),cos(a))*p.xz;
   p.xy+=3.14159*(abs(2.*fract(s*p.xy/6.28318+.75)-1.)-.5)/s;
   s*=3.;
  }
  vec3 ap=abs(x);
  float stepSize=max(min(20.-ap.x,30.-ap.y),max(.8*abs(m),.01+1.6*abs(2.+2.*dot(sin(vec3(time)+p/12.),cos(p.yzx/16.)))));
  stepSize=clamp(stepSize,.025,10.);
  dist+=stepSize;
  c+=(1.+cos(.1*float(i)+vec3(1.,3.,4.)))/stepSize;
 }
 c.b*=1.3;
 vec3 tunnel=tanh(c*c/3e5);
 // Star Nest: 12 volume samples x 10 fractal folds (vs 20 x 17).
 vec3 dir=vec3(q*.8,1.)*(.2+c.b);
 vec3 from=vec3(1.,.5,.5)+vec3(c.b);
 float z=.1,fade=1.;
 vec3 v=vec3(0.);
 for(int j=0;j<12;j++){
  vec3 p=from+z*dir*.5;
  p=abs(vec3(tile)-mod(p,vec3(tile*2.)));
  float prev=0.,acc=0.;
  for(int k=0;k<10;k++){
   p=abs(p)/max(dot(p,p),.0001)-formuparam;
   float len=length(p);
   acc+=abs(len-prev);prev=len;
  }
  float dm=max(0.,.3-acc*acc*.001);
  acc=min(acc*acc*acc,1e5);
  if(j>6)fade*=1.3-dm;
  v+=vec3(fade);
  v+=vec3(z,z*z,pow(z,4.))*acc*.0015*fade;
  fade*=.73;z+=.1;
 }
 v=mix(vec3(length(v)),v,.85);
 vec3 color=v*tunnel*(.018+.006*c);
 color=1.-exp(-max(color,vec3(0.))*.22);
 outColor=vec4(clamp(color,0.,1.),pc.parameters.w);
}
