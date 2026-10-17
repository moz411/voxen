#version 450
// Adapted from the user-provided Shadertoy code (iTime/iResolution -> push constants/UV).
layout(location=0) in vec2 uv;
layout(location=0) out vec4 outColor;
layout(push_constant) uniform PushConstants { mat4 mvp; vec4 parameters; } pc;
float random(float x){return fract(sin(x)*1e4);}
float random(vec2 st){return fract(sin(dot(st,vec2(12.9898,78.233)))*43758.5453123);}
mat2 rotate2D(float angle){return mat2(cos(angle),sin(angle),-sin(angle),cos(angle));}
float flower(vec2 u,vec2 center,float r,int petals,float smoothR){
    vec2 v=u-center;float d=length(v);float theta=atan(v.y,v.x);
    float curve=abs(sin(theta*float(petals)))*(r*0.85);
    return 1.0-smoothstep(r-curve-smoothR,r-curve,d);
}
void main(){
    const vec2 TILES=vec2(400.,300.);
    const float PI=3.1415;
    float time=pc.parameters.x*pc.parameters.y;
    // Use wall local UV, preserving animation in the object's surface space.
    vec2 st=(uv*0.5+0.5)*TILES;
    vec2 ipos=floor(st);
    vec3 color=vec3(0.);
    vec2 vel=vec2(0.,time*(0.08+random(ipos.x))*100.);
    vec2 anim=floor(st+vel);
    color.g=random(anim)-random(ipos.x);
    vec2 rot=rotate2D(sin(time*0.2)*PI)*(st-TILES*0.5)+TILES*0.5;
    float f1=flower(rot,TILES*0.5,TILES.x*0.9,4,100.);
    rot=rotate2D(sin(time*0.2+32.14)*-PI)*(st-TILES*0.5)+TILES*0.5;
    float f2=flower(rot,TILES*0.5,TILES.x*0.9,2,100.);
    color.g+=0.3*f1;
    color.g-=0.35*f2;
    color.r=step(0.96,color.g);
    color.b=step(0.9,color.g);
    float sm=0.4;
    color=mix(vec3(0.),color,smoothstep(0.,sm,fract(st.x)));
    color=mix(color,vec3(0.),smoothstep(1.-sm,1.,fract(st.x)));
    outColor=vec4(clamp(color,0.,1.),clamp(pc.parameters.w,0.,0.9));
}