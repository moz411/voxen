#version 450
// Cosmic Palette (Shadertoy mtyGWy) adapted from user-supplied GLSL.
// Palette reference: https://iquilezles.org/articles/palettes/
layout(location=0) in vec2 uv;
layout(location=0) out vec4 outColor;
layout(push_constant) uniform PushConstants {mat4 mvp;vec4 parameters;} pc;
vec3 palette(float t) {
    vec3 a=vec3(0.5);
    vec3 b=vec3(0.5);
    vec3 c=vec3(1.0);
    vec3 d=vec3(0.263,0.416,0.557);
    return a+b*cos(6.28318*(c*t+d));
}
void main(){
    vec2 p=uv; // normalized local wall coordinates, independent of eye resolution
    vec2 initial=p;
    vec3 color=vec3(0.0);
    float time=pc.parameters.x*pc.parameters.y;
    for(int i=0;i<4;++i){
        p=fract(p*1.5)-0.5;
        float d=length(p)*exp(-length(initial));
        vec3 layer=palette(length(initial)+float(i)*0.4+time*0.4);
        d=abs(sin(d*8.0+time)/8.0);
        d=pow(0.01/max(d,0.0015),1.2); // prevent infinities on mobile GPU
        color+=layer*d;
    }
    // Soft tonemapping prevents extreme HDR values and preserves the colors.
    color=1.0-exp(-color);
    outColor=vec4(color,pc.parameters.w);
}
