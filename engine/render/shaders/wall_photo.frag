#version 450
layout(set=0,binding=0) uniform sampler2D wallPhoto;
layout(push_constant) uniform Push {mat4 mvp;vec4 settings;} pc;
// settings: wall aspect ratio, texture aspect ratio, cover=1 / fit=0, fade
layout(location=0) in vec2 uv;
layout(location=0) out vec4 outColor;
void main(){
 float wr=max(pc.settings.x,0.001),ir=max(pc.settings.y,0.001);
 vec2 mapped=uv;
 if(pc.settings.z>0.5){ // COVER: crop longest dimension
   if(wr>ir)mapped.y=(uv.y-0.5)*(ir/wr)+0.5;
   else mapped.x=(uv.x-0.5)*(wr/ir)+0.5;
 }else{ // FIT: preserve whole image with white letterbox
   if(wr>ir)mapped.x=(uv.x-0.5)*(wr/ir)+0.5;
   else mapped.y=(uv.y-0.5)*(ir/wr)+0.5;
 }
 vec3 rgb=(mapped.x>=0.0&&mapped.x<=1.0&&mapped.y>=0.0&&mapped.y<=1.0)
   ?texture(wallPhoto,vec2(mapped.x,1.0-mapped.y)).rgb:vec3(1.0);
 outColor=vec4(mix(vec3(1.0),rgb,clamp(pc.settings.w,0.0,1.0)),1.0);
}
