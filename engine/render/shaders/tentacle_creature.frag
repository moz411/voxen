#version 450
layout(location=0) in vec3 vNormal;
layout(location=1) in vec3 vPosition;
layout(location=2) in float vPulse;
layout(location=0) out vec4 outColor;
layout(push_constant) uniform Push {mat4 mvp;vec4 parameters;} pc;
void main(){
 float t=pc.parameters.x*max(pc.parameters.y,0.01);
 vec3 n=normalize(vNormal);
 float light=0.40+0.60*max(dot(n,normalize(vec3(-0.4,0.75,0.6))),0.0);
 float pulse=0.5+0.5*sin(t*2.0);
 vec3 base=mix(vec3(0.74,0.43,0.28),vec3(0.9,0.6,0.25),pulse);
 if(vPulse>0.5){
  base=mix(vec3(0.60,0.30,0.17),vec3(0.98,0.68,0.3),pulse);
  base*=0.75+0.25*sin(length(vPosition)*12.0-t*2.0);
 }else{
  // Seven glowing eyes painted on the upper-facing sphere surface.
  const vec3 eyeDir[7]=vec3[7](
   vec3(-0.45,0.29,0.84),vec3(0.40,0.28,0.87),
   vec3(0.0,0.56,0.82),vec3(-0.12,-0.17,0.98),
   vec3(0.33,-0.35,0.88),vec3(-0.52,-0.34,0.79),
   vec3(0.53,0.58,0.62));
  for(int i=0;i<7;i++){
   float d=length(n-normalize(eyeDir[i]));
   float radius=0.11+float(i%3)*0.025;
   float eye=1.0-smoothstep(radius,radius+0.055,d);
   float pupil=1.0-smoothstep(0.028,0.057,length(n-normalize(eyeDir[i]+vec3(0.035*sin(t*1.2+float(i)),0.025*cos(t+float(i)),0))));
   base=mix(base,vec3(0.9,0.98,0.75),eye);
   base=mix(base,vec3(0.02,0.06,0.12),pupil*eye);
  }
 }
 outColor=vec4(base*light+vec3(0.10,0.06,0.02)*pow(1.0-abs(n.z),3.0),1.0);
}
