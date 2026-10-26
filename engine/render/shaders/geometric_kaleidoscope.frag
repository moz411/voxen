#version 450
// Animated geometric kaleidoscope based on user Shadertoy.
layout(location=0) in vec2 uv;
layout(location=0) out vec4 outColor;
layout(push_constant) uniform Push {mat4 mvp;vec4 parameters;} pc;
const float TAU=6.28318530718;
mat2 rot(float a){float c=cos(a),s=sin(a);return mat2(c,-s,s,c);}
vec3 hue(float h){return clamp(0.6+0.6*cos(h+vec3(0.0,2.3,4.2)),0.0,1.0);}
vec2 grid(vec2 p){return mat2(6.,0.,3.,5.)*p;}
void main(){
 float t=pc.parameters.x*max(pc.parameters.y,0.01);
 // Preserve the six-panel Shadertoy demo over each tracked wall.
 vec2 coord=(uv+vec2(1.0))*1.55;
 vec2 windowId=floor(coord);
 vec2 p=fract(coord)-0.5;
 int id=int(windowId.x+3.0*windowId.y);
 vec3 col=vec3(0.02,0.025,0.045);
 if(id<=1){
  vec2 q=grid(p-vec2(0.2*t));
  vec2 tiled=fract(q);
  vec2 band=fract(vec2(q.x-q.y,q.y));
  float line=1.-smoothstep(0.025,0.055,min(min(band.x,1.-band.x),min(band.y,1.-band.y)));
  col=id==0?hue(q.x*0.75-t*0.3)*0.65:mix(vec3(0.02,0.04,0.12),hue(tiled.x*TAU),line);
 }else if(id==2){
  vec2 q=rot(t)*p;
  float v=length(q-0.2*p/max(length(p),0.02));
  float line=1.-smoothstep(0.02,0.05,abs(sin(v*18.0)));
  col=hue(v*4.0-t)*line;
 }else if(id==3){
  float d=dot(p,vec2(cos(t),sin(t)));
  float line=1.-smoothstep(0.03,0.07,abs(sin(d*26.0)));
  col=hue(d*6.0-t)*line;
 }else if(id==4){
  col=hue(TAU*length(p)*2.0-t);
 }else if(id==5){
  col=hue(atan(p.y,p.x)-t);
 }else{
  vec2 g=ceil(grid(p));
  col=hue(dot(g,vec2(1.,7.))-t);
 }
 float separator=1.-smoothstep(0.0,0.012,min(min(fract(coord.x),1.-fract(coord.x)),min(fract(coord.y),1.-fract(coord.y))));
 col=mix(col,vec3(0.08,0.12,0.21),separator*0.65);
 outColor=vec4(col,clamp(pc.parameters.w,0.,0.6));
}
