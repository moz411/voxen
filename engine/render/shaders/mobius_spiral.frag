#version 450
// MOBIUS SPIRAL — procedural warp-grid adaptation of the supplied Shadertoy.
// Stateless fragment shader, no samplers/extra render targets. Quest friendly.
layout(location=0) in vec2 uv;
layout(location=0) out vec4 outColor;
layout(push_constant) uniform PushConstants {
    mat4 mvp;
    vec4 parameters; // time, speed, unused, opacity
} pc;

const float TAU=6.28318530718;
const vec2 Z1=vec2(-0.875,-0.525);
const vec2 Z2=vec2(0.375,-0.125);
vec2 mobius(vec2 p) {
    vec2 a=p-Z1, b=p-Z2;
    float denom=max(dot(b,b),1e-5);
    return mat2(a,a.y,-a.x)*b/denom;
}
vec2 spiralZoom(vec2 p,float t) {
    p-=vec2(-0.5);
    float angle=atan(p.y,p.x)/TAU+t/32.0;
    float distanceLog=log(max(length(p),1e-5));
    return mat2(5.0,2.5,0.6283185,-0.5)*vec2(angle,distanceLog)
           +vec2(-1.0,1.0)*t*0.125;
}
// Multi-scale anti-aliased isolines in the transformed coordinate domain.
float isoLines(vec2 q,float scale) {
    vec2 p=q*scale;
    vec2 dd=max(fwidth(p),vec2(0.001));
    vec2 dist=abs(fract(p+0.5)-0.5);
    vec2 line=1.0-smoothstep(vec2(0.0),max(dd*1.65,vec2(0.015)),dist);
    return max(line.x,line.y);
}
void main() {
    float time=pc.parameters.x*max(pc.parameters.y,0.01);
    vec2 p=uv*1.9;
    vec2 warp=spiralZoom(mobius(p),time);
    // Continuous mip transition avoids shimmer as the mapping expands.
    vec2 fw=max(fwidth(warp),vec2(0.0001));
    float footprint=max(fw.x,fw.y);
    float logScale=clamp(log2(0.16/max(footprint,1e-4)),-2.0,6.0);
    float level=floor(logScale);
    float mixLOD=fract(logScale);
    float s=exp2(level);
    float grid=mix(isoLines(warp,s),isoLines(warp,s*2.0),mixLOD);
    float finer=isoLines(warp,s*0.5);
    float lines=clamp(grid*0.85+finer*0.22,0.0,1.0);
    vec3 background=vec3(0.008,0.012,0.032);
    vec3 cool=vec3(0.11,0.64,1.0);
    vec3 warm=vec3(0.95,0.33,0.78);
    vec3 tint=mix(cool,warm,0.5+0.5*sin(warp.x*0.8+time*0.2));
    float halo=pow(clamp(lines,0.0,1.0),0.6);
    vec3 col=background+tint*(lines*0.85+halo*0.12);
    outColor=vec4(col,clamp(pc.parameters.w,0.0,0.6));
}
