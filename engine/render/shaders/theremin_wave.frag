#version 450
layout(location=0) in vec2 uv;
layout(location=0) out vec4 outColor;
layout(push_constant) uniform PushConstants { mat4 mvp; vec4 parameters; } pc;
const float TAU=6.28318530718;
float pitch(float t){
 if(t<15.)return -1.;
 if(t<16.)return 31.; if(t<17.)return 56.; if(t<22.)return 55.;
 if(t<23.)return -1.; if(t<24.)return 58.; if(t<25.)return 42.;
 if(t<30.)return 43.; if(t<33.)return -1.;
 if(t<34.5)return 43.; if(t<35.5)return 39.; if(t<37.)return 31.;
 if(t<39.)return 34.; if(t<39.5)return 32.; if(t<40.5)return 31.;
 if(t<41.)return -1.; if(t<42.5)return 31.; if(t<43.)return 44.;
 if(t<46.)return 43.; return -1.;
}
float sound(float t){
 float beat=mod(t*2.4+1.33,48.);
 float n=pitch(beat);
 if(n<0.)return 0.;
 float f=55.*exp2((n-3.)/12.);
 float phase=TAU*f*t;
 return 0.4*sin(phase+(beat>=32.?1.5707963:0.)*sin(2.*phase));
}
void main(){
 float t=pc.parameters.x*max(pc.parameters.y,0.01);
 float y=sound(t+uv.x/45.);
 float dist=abs(uv.y-y);
 float w=max(fwidth(y),0.008);
 float line=1.-smoothstep(w,w+0.018,dist);
 float glow=exp(-dist*30.)*0.18;
 vec3 color=vec3(0.012,0.025,0.045)+vec3(0.11,0.95,0.89)*line+vec3(0.14,0.42,0.87)*glow;
 outColor=vec4(min(color,vec3(1.)),clamp(pc.parameters.w,0.,0.6));
}
