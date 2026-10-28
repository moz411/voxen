#version 450
// TAN GLITCH — port of user-provided Shadertoy bands/distortion shader.
// Limits tangent poles to avoid uncontrolled GPU infinities on Vulkan.
layout(location=0) in vec2 uv;
layout(location=0) out vec4 outColor;
layout(push_constant) uniform PushConstants {mat4 mvp;vec4 parameters;} pc;
float safeTan(float x){
 float c=cos(x);
 return clamp(sin(x)/(sign(c)*max(abs(c),.035)),-25.,25.);
}
void main(){
 float time=pc.parameters.x*pc.parameters.y;
 float clock=time/5.;
 float ttime=floor(time)+pow(max(fract(time),.0001),clamp(sin(time*1.333)*.5,-.5,.5));
 vec2 p=uv*.7,original=p;
 p.x+=safeTan(abs(p.x)*5.);
 float d=fract(10.*p.x+clock);
 d=smoothstep(.2,.1,d);
 float streak=safeTan(p.x+time);
 vec3 low=vec3(.1);
 vec3 high=vec3(streak*.1+step(.1,abs(cos(clock*2.))*p.x),streak*.9,.1+step(1.,sin(clock)*sin(clock)*abs(p.x)));
 vec3 col=mix(low,high,d);
 col=clamp(col,0.,1.);
 // Original alpha pulses with d*sin(...) but background shader output
 // should remain valid even for negative oscillations.
 float pulse=d*sin(ttime+1.-floor(length(floor(original*10.))));
 col+=vec3(.12,.03,.22)*max(0.,pulse);
 outColor=vec4(col,pc.parameters.w);
}
