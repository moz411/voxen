#version 450
// Chromatic Edges, procedural single-pass approximation of the supplied
// Shadertoy postprocess. Original BufA is unavailable: no claim of equivalence.
// Reference: https://www.shadertoy.com/view/NXySRD
// Source credit Danil; original tagged CC BY-NC-SA 3.0.
layout(location=0) in vec2 uv;
layout(location=0) out vec4 outColor;
layout(push_constant) uniform PushConstants {mat4 mvp;vec4 parameters;} pc;
vec3 palette(float d){return 0.5+0.5*cos(6.2831853*(d+vec3(0.0,0.33,0.67)));}
float field(vec2 p,float t){
    vec2 q=p;
    float f=0.0,amp=0.5;
    for(int i=0;i<4;i++){
        q=mat2(0.8,-0.6,0.6,0.8)*q*1.78+vec2(0.7,-0.4)*sin(t*0.16);
        f+=amp*(0.5+0.5*sin(5.0*q.x+cos(4.0*q.y+t*0.65)));
        amp*=0.5;
    }
    return f;
}
vec3 sourceColor(vec2 p,float t){
    float f=field(p,t);
    vec3 c=palette(f*0.8+t*0.025);
    return c*pow(clamp(f,0.0,1.0),1.3);
}
void main(){
    vec2 p=uv;
    float t=pc.parameters.x*pc.parameters.y;
    const float h=0.007;
    vec3 center=sourceColor(p,t);
    vec3 dx=sourceColor(p+vec2(h,0),t)-sourceColor(p-vec2(h,0),t);
    vec3 dy=sourceColor(p+vec2(0,h),t)-sourceColor(p-vec2(0,h),t);
    float edge=clamp(2.6*length(dx)+2.6*length(dy),0.0,1.0);
    // Approximation of the original HSV-saturated edge blending.
    float gray=dot(center,vec3(0.2126,0.7152,0.0722));
    vec3 saturated=clamp(mix(vec3(gray),center,2.0)*1.75,0.0,1.0);
    vec3 blurred=center*center;
    vec3 orig=mix(blurred,saturated,edge);
    vec3 col=clamp(sqrt(max(orig,vec3(0.0)))*0.32+orig*1.21,0.0,1.0);
    float alpha=clamp(max(dot(col,vec3(0.5)),edge*0.25),0.0,1.0);
    outColor=vec4(col,alpha*pc.parameters.w);
}
