#version 450
// Animated Mandelbrot, adapted from Inigo Quilez (iq/2013)
// https://iquilezles.org/articles/msetsmooth
// https://www.youtube.com/c/InigoQuilez
// Original Shadertoy's 512 iterations / AA 2 was reduced for Quest 3.
// Finite precision prevents mathematically unlimited zoom. This version
// cycles smoothly through a deep-looking zoom range instead of stalling.
layout(location=0) in vec2 uv;
layout(location=0) out vec4 outColor;
layout(push_constant) uniform PushConstants {mat4 mvp;vec4 parameters;} pc;
const float TAU=6.283185307179586;
float mandelbrot(vec2 c,float time){
    float c2=dot(c,c);
    if(256.0*c2*c2-96.0*c2+32.0*c.x-3.0<0.0)return 0.0;
    if(16.0*(c2+2.0*c.x+1.0)-1.0<0.0)return 0.0;
    const float B=256.0;
    vec2 z=vec2(0.0);
    float n=0.0;
    for(int i=0;i<128;i++){
        z=vec2(z.x*z.x-z.y*z.y,2.0*z.x*z.y)+c;
        if(dot(z,z)>B*B)break;
        n+=1.0;
    }
    if(n>=128.0)return 0.0;
    float sn=n-log2(max(log2(dot(z,z)),0.00001))+4.0;
    float smoothMix=smoothstep(-0.1,0.0,sin(3.1415927*time));
    return mix(n,sn,smoothMix);
}
void main(){
    float time=pc.parameters.x*pc.parameters.y;
    // Logarithmic 30s zoom cycle; no reverse zoom. Reset is hidden
    // at the same target, preserving the feeling of infinite movement.
    float progress=fract(time/30.0);
    float zoo=exp2(-12.0*progress);
    float angle=0.10*time;
    mat2 rot=mat2(cos(angle),sin(angle),-sin(angle),cos(angle));
    vec2 p=uv*2.0;
    vec2 c=vec2(-0.745,0.186)+zoo*(rot*p);
    float sn=mandelbrot(c,time);
    vec3 col=vec3(0.0);
    if(sn>0.0){
        float nor=1.0+log2(1.0/zoo);
        col=0.5+0.5*cos(0.2*sn/nor+vec3(2.7,3.2,3.7));
    }
    outColor=vec4(col,clamp(pc.parameters.w,0.0,0.6));
}
