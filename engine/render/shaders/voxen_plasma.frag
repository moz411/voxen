#version 450
layout(location=0) in vec2 uv;
layout(location=0) out vec4 outColor;
layout(push_constant) uniform PushConstants {mat4 mvp;vec4 parameters;} pc;
// x = time, y = speed, z = hue, w = density
void main(){
    vec2 p=uv;
    float r=length(p);
    if(r>0.98)discard;
    float t=pc.parameters.x*pc.parameters.y;
    float density=pc.parameters.w;
    float a=atan(p.y,p.x);
    float swirls=sin(9.0*a+3.0*sin(r*12.0-t*1.6)+t*1.3);
    float filaments=sin(24.0*r-5.0*a-t*2.1);
    float energy=exp(-r*r*3.3)*max(0.0,0.5+0.32*swirls+0.25*filaments);
    energy=pow(energy,max(0.3,2.1-density));
    vec3 cold=0.35+0.35*cos(6.2831853*(pc.parameters.z+vec3(0.0,0.22,0.48)));
    vec3 hot=vec3(0.95,0.85,1.0);
    vec3 col=mix(cold,hot,clamp(energy*1.3,0.0,1.0))*(0.18+energy*2.4);
    float alpha=clamp(energy*2.5,0.0,0.90)*(1.0-smoothstep(0.65,0.98,r));
    if(alpha<0.014)discard;
    outColor=vec4(col,alpha);
}
