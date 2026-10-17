#version 450
// PHI FIELD — standalone Quest-friendly interpretation of the supplied Shadertoy fragment.
// Reference: https://www.shadertoy.com/view/ssGczz ; remake https://www.shadertoy.com/view/X3VBR3
// The original relies on external gm/ds helpers, PI/TAU/SCALE/R and mouse input.
// Here gm/ds are reconstructed as animated contour and logarithmic spiral fields.
layout(location=0) in vec2 uv;
layout(location=0) out vec4 outColor;
layout(push_constant) uniform PushConstants {mat4 mvp;vec4 parameters;} pc;
const float PI=3.141592653589793;
const float TAU=6.283185307179586;
float contour(float v,float phase,float width){
    float f=abs(fract(v*0.16+phase)-0.5);
    return 1.0-smoothstep(width,width+0.02,f);
}
float spiral(vec2 p,float handed,float t,float offset,float px){
    float angle=atan(p.y,p.x);
    float radius=max(length(p),0.001);
    float v=(angle*handed+2.0*log(radius)+offset)/TAU-t;
    float d=abs(fract(v*3.0)-0.5);
    return 1.0-smoothstep(0.015+px,0.045+px,d);
}
void main(){
    float t=pc.parameters.x*pc.parameters.y/PI*2.0;
    vec2 p=uv*2.0;
    float px=max(length(fwidth(p)),0.002);
    float x=p.x,y=p.y;
    float l=length(p);
    float safeY=sign(y+0.000001)*max(abs(y),0.005);
    float safeX=sign(x+0.000001)*max(abs(x),0.005);
    float mc=(x*x+y*y-1.0)/safeY;
    float g=min(abs(mc),1.0/max(abs(mc),0.005));
    g=clamp(g,0.0,1.0);
    vec3 gold=vec3(1.0,0.6,0.0)*g*l;
    vec3 blue=vec3(0.3,0.5,0.9)*(1.0-g);
    vec3 rgb=max(gold,blue);
    vec3 c=vec3(0.0);
    c=max(c,rgb*contour(mc,-t*0.11,0.08));
    c=max(c,rgb*contour(abs(y/safeX)*sign(y),-t*0.11,0.08));
    c=max(c,rgb*contour((x*x)/(safeY*safeY)*sign(y),-t*0.11,0.08));
    c=max(c,rgb*contour(x*x+y*y,t*0.11,0.08));
    float sp=spiral(p,1.0,t/TAU,0.0,px)+spiral(p,1.0,t/TAU,PI,px)
            +spiral(p,-1.0,t/TAU,0.0,px)+spiral(p,-1.0,t/TAU,PI,px);
    c+=rgb*sp*0.35;
    c+=vec3(pow(max(1.0-l,0.0),3.0));
    c=max(c,vec3(0.0));
    c*=sqrt(c)*1.5;
    outColor=vec4(clamp(c,0.0,1.0),clamp(pc.parameters.w,0.0,0.6));
}
