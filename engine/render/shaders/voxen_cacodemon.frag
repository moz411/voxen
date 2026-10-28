#version 450
// CACODEMON — standalone stylized interpretation of Tonny Espeset's
// "Cacodemon" (2026): https://www.shadertoy.com/view/NXtXzj
// Credit and source link retained as requested by the author.
// Not a faithful port: original relies on Common, multiple buffers,
// 3D distance cache, shadow passes and precomputed noise.
layout(location=0) in vec2 uv;
layout(location=0) out vec4 outColor;
layout(push_constant) uniform PushConstants {mat4 mvp;vec4 parameters;} pc;
const float PI=3.141592653589793;
float hash21(vec2 p){return fract(sin(dot(p,vec2(127.1,311.7)))*43758.5453);}
float sdEllipse(vec2 p,vec2 radius){return (length(p/radius)-1.)*min(radius.x,radius.y);}
float segment(vec2 p,vec2 a,vec2 b){
 vec2 q=p-a,v=b-a;return length(q-v*clamp(dot(q,v)/max(dot(v,v),.0001),0.,1.));
}
float spikeShape(vec2 p, float a, float len){
 vec2 d=vec2(cos(a),sin(a)),v=vec2(-d.y,d.x);
 float u=dot(p,d),w=abs(dot(p,v));
 return max(max(-u,u-len),w-(.12*(1.-u/max(len,.001))+.012));
}
float sdBody(vec2 p,float t){
 float wobble=.024*sin(10.*atan(p.y,p.x)+t*1.7);
 return length(p)-(.54+wobble);
}
void main(){
 float t=pc.parameters.x*pc.parameters.y;
 vec2 p=uv*1.05;
 p.y-=.025*sin(t*1.4);
 p.x+=.015*sin(t*.8);
 float body=sdBody(p,t);
 float spikes=1e4;
 for(int i=0;i<13;i++){
  float fi=float(i),a=fi*6.2831853/13.+.10*sin(fi*13.7);
  vec2 dir=vec2(cos(a),sin(a));
  spikes=min(spikes,spikeShape(p-dir*.45,a,.23+.16*hash21(vec2(fi,4.))));
 }
 float silhouette=min(body,spikes);
 float feather=.007;
 float mask=1.-smoothstep(-feather,feather,silhouette);
 if(mask<.006)discard;
 vec3 color=vec3(0.);
 // Ember-red leathery face with subtle irregular mottling.
 float radial=length(p);
 float n=hash21(floor(p*50.));
 float shade=clamp(1.-radial*.52+.15*p.y,0.,1.);
 vec3 skin=mix(vec3(.24,.013,.012),vec3(.87,.125,.055),shade);
 skin*=.88+.24*n;
 color=skin;
 // Single giant glossy eye with iris and dark slit.
 vec2 eyeCenter=vec2(0.,.16);
 vec2 ep=p-eyeCenter;
 float eye=sdEllipse(ep,vec2(.265,.235));
 float eyeMask=1.-smoothstep(-.01,.012,eye);
 vec3 sclera=mix(vec3(.57,.33,.23),vec3(1.,.83,.65),clamp(1.-length(ep)*2.,0.,1.));
 color=mix(color,sclera,eyeMask);
 float irisR=length(ep-vec2(.03*sin(t*.6),.012*cos(t*.45)));
 float iris=1.-smoothstep(.143,.158,irisR);
 float ang=atan(ep.y,ep.x);
 vec3 irisCol=mix(vec3(.05,.10,.025),vec3(.57,.61,.15),.5+.5*sin(ang*44.+irisR*95.));
 color=mix(color,irisCol,iris*eyeMask);
 float pupil=1.-smoothstep(.039,.048,length((ep-vec2(.03*sin(t*.6),.012*cos(t*.45)))/vec2(.6,1.)));
 color=mix(color,vec3(.002,.002,.002),pupil*eyeMask);
 float glint=1.-smoothstep(.024,.044,length(ep-vec2(-.10,.105)));
 color+=vec3(.95,.85,.75)*glint*eyeMask*.8;
 // Frowning open mouth, sharp teeth and breathing blue/orange charge.
 vec2 mouthP=p-vec2(0.,-.285);
 float mouth=sdEllipse(mouthP,vec2(.29,.12+.035*sin(t*2.));
 float mouthMask=1.-smoothstep(-.007,.009,mouth);
 color=mix(color,vec3(.035,.002,.014),mouthMask);
 for(int k=0;k<9;k++){
  float x=(float(k)-4.)*.058;
  float tooth=length(vec2((mouthP.x-x)*1.8,(mouthP.y+.048)*.8));
  float tip=1.-smoothstep(.019,.032,tooth);
  color=mix(color,vec3(1.,.84,.55),tip*mouthMask);
 }
 float cycle=fract(t*.17);
 float charge=smoothstep(.18,.75,cycle)*(1.-smoothstep(.78,.95,cycle));
 float gas=exp(-dot(mouthP,mouthP)*80.)*charge;
 color+=vec3(.08,.6,2.)*gas*.9;
 // Projected fireball leaving mouth periodically, along the right edge.
 vec2 firePos=vec2(.08+(cycle-.77)*2.0,-.32);
 float fireRadius=mix(.045,.17,clamp((cycle-.77)*4.,0.,1.));
 float fire=1.-smoothstep(fireRadius,fireRadius+.065,length(p-firePos));
 float launch=smoothstep(.75,.79,cycle);
 vec3 flame=mix(vec3(.15,.9,2.2),vec3(2.8,.36,.015),smoothstep(.8,.94,cycle));
 color+=flame*fire*launch;
 float alpha=max(mask,fire*launch*.9);
 color=1.-exp(-max(color,vec3(0.)));
 outColor=vec4(clamp(color,0.,1.),alpha*pc.parameters.w);
}
