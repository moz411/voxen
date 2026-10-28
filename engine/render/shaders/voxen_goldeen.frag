#version 450
// GOLDEEN — independent Quest 3 adaptation of "Goldeen Shader" by Noztol.
// User supplied original uses 120-step tracing, 3D bubbles and detailed SDF.
// This version retains the expressive fish silhouette, flowing three tails,
// side/dorsal fins, forehead horn, blinking eyes and rising bubbles.
layout(location=0) in vec2 uv;
layout(location=0) out vec4 outColor;
layout(push_constant) uniform PushConstants { mat4 mvp; vec4 parameters; } pc;
float time;
float sdEllipsoid(vec3 p,vec3 r){float k0=length(p/r),k1=length(p/(r*r));return k0*(k0-1.)/max(k1,.0001);}
float sdCapsule(vec3 p,vec3 a,vec3 b,float r){
 vec3 pa=p-a,ba=b-a;return length(pa-ba*clamp(dot(pa,ba)/max(dot(ba,ba),.0001),0.,1.))-r;
}
float smoothMin(float a,float b,float k){
 float h=clamp(.5+.5*(b-a)/k,0.,1.);return mix(b,a,h)-k*h*(1.-h);
}
mat2 rot(float a){float s=sin(a),c=cos(a);return mat2(c,s,-s,c);}
vec2 mapFish(vec3 p){
 // Mouth faces camera (positive Z); x is left/right.
 p.y+=.025*sin(time*2.);
 p.xz=rot(.06*sin(time*1.5))*p.xz;
 float body=sdEllipsoid(p,vec3(.36,.29,.53));
 float head=sdEllipsoid(p-vec3(0.,.075,.25),vec3(.31,.30,.30));
 float d=smoothMin(body,head,.1);
 float mat=1.;
 // Forward-pointing tapered horn.
 vec3 horn=p-vec3(0.,.30,.31);
 float hy=clamp(horn.y,0.,.34);
 float radius=.08*(1.-hy/.34);
 float cone=max(length(horn.xz-vec2(hy*.32,hy*.05))-radius,max(-horn.y,horn.y-.34));
 if(cone<d){d=cone;mat=2.;}
 // Three flexible translucent-looking back tail lobes.
 for(int i=0;i<3;i++){
  float fi=float(i)-1.;
  vec3 q=p-vec3(fi*.13,.01,-.47);
  q.x+=.20*sin(time*4.+fi*1.2-q.z*5.)*smoothstep(.02,.57,-q.z);
  q.xy=rot(fi*.38)*q.xy;
  float taper=clamp((-q.z+.02)/.65,0.,1.);
  float fin=max(abs(q.x)-(.025+.20*taper),max(abs(q.y)-(.06+.14*taper),max(q.z,-q.z-.65)));
  fin=max(fin,abs(q.x)*.12+abs(q.y)*.05-.030);
  if(fin<d){d=fin*.72;mat=3.;}
 }
 // Dorsal fin and pectorals.
 vec3 dorsal=p-vec3(0.,.27,-.08);
 dorsal.x+=.03*sin(time*3.+dorsal.z*5.);
 float df=max(abs(dorsal.x)-.018,max(abs(dorsal.z)-.18,abs(dorsal.y-.15)-.15));
 if(df<d){d=df;mat=3.;}
 for(int i=0;i<2;i++){
  float sg=i==0?-1.:1.;
  vec3 f=p-vec3(sg*.32,-.06,.04);
  f.yz=rot(sg*(.35+.2*sin(time*5.)))*f.yz;
  float fin=max(abs(f.x)-.025,max(abs(f.y)-.18,abs(f.z)-.13));
  if(fin<d){d=fin;mat=3.;}
 }
 // Prominent white eyes.
 for(int i=0;i<2;i++){
  float sg=i==0?-1.:1.;
  float eye=length(p-vec3(sg*.245,.14,.31))-.126;
  if(eye<d){d=eye;mat=4.;}
 }
 // Small smiling lips.
 float lips=sdEllipsoid(p-vec3(0.,-.09,.525),vec3(.13,.055,.055));
 if(lips<d){d=lips;mat=5.;}
 return vec2(d,mat);
}
vec3 normalAt(vec3 p){
 float e=.005;
 return normalize(vec3(mapFish(p+vec3(e,0,0)).x-mapFish(p-vec3(e,0,0)).x,
 mapFish(p+vec3(0,e,0)).x-mapFish(p-vec3(0,e,0)).x,
 mapFish(p+vec3(0,0,e)).x-mapFish(p-vec3(0,0,e)).x));
}
float bubble(vec2 p,vec2 center,float radius){
 return abs(length(p-center)-radius);
}
void main(){
 time=pc.parameters.x*pc.parameters.y;
 vec2 p=uv*.85;
 vec3 ro=vec3(0.,.08,2.45),rd=normalize(vec3(p,-1.65));
 float t=0.,mat=0.;bool hit=false;
 for(int i=0;i<56;i++){
  vec3 pos=ro+rd*t;
  vec2 scene=mapFish(pos);
  if(scene.x<.003){hit=true;mat=scene.y;break;}
  t+=clamp(scene.x*.72,.007,.13);
  if(t>4.5)break;
 }
 vec3 col=vec3(0.);
 float alpha=0.;
 if(hit){
  vec3 pos=ro+rd*t,n=normalAt(pos);
  float dif=max(dot(n,normalize(vec3(.5,.85,1.))),0.);
  float fres=pow(1.-max(dot(-rd,n),0.),3.);
  vec3 base=vec3(.98,.59,.18);
  if(mat<1.5)base=mix(vec3(1.,.37,.025),vec3(1.,.95,.72),smoothstep(-.25,.33,pos.y));
  else if(mat<2.5)base=vec3(1.,.93,.77);
  else if(mat<3.5)base=mix(vec3(1.,.32,.05),vec3(1.,.93,.67),smoothstep(-.4,.4,pos.y));
  else if(mat<4.5){
   base=vec3(.98,.96,.91);
   vec3 center=vec3(sign(pos.x)*.245,.14,.31);
   vec2 eye=(pos-center).xy;
   float pupil=length(eye-vec2(.018*sin(time*.6),-.015));
   if(pupil<.060)base=vec3(.14,.66,.22);
   if(pupil<.029)base=vec3(.015);
  } else base=vec3(1.,.32,.5);
  col=base*(.23+.75*dif)+vec3(1.,.85,.65)*pow(max(dot(reflect(rd,n),normalize(vec3(-.5,.8,1.))),0.),26.)*.35;
  col+=fres*vec3(.23,.37,.47);
  alpha=.98;
 }
 // Bubbles are transparent outlined rings, clipped against the fish.
 for(int j=0;j<9;j++){
  float fj=float(j);
  float y=fract(fj*.173+time*.11);
  vec2 center=vec2(-.58+.22*sin(fj*7.1+time*.6),-.75+y*1.6);
  float radius=.021+.014*fract(fj*.371);
  float edge=bubble(p,center,radius);
  float b=1.-smoothstep(.002,.006,edge);
  if(!hit){col+=vec3(.30,.8,1.)*b*.65;alpha=max(alpha,b*.65);}
 }
 if(alpha<.01)discard;
 outColor=vec4(clamp(col,0.,1.),alpha*pc.parameters.w);
}
