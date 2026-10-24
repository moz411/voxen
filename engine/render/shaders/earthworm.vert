#version 450
// EARTHWORM: 12 animated overlapping-sphere centers from the supplied
// Shadertoy distance field, converted to a lightweight continuous tube mesh.
// No 200-step raymarching and no iChannel0 dependency on Quest.
layout(push_constant) uniform PushConstants {
    mat4 mvp;
    vec4 parameters; // time, speed, material/hue, deformation
} pc;
layout(location=0) out vec3 vNormal;
layout(location=1) out vec3 vPosition;
layout(location=2) out float vPulse;
const float PI=3.141592653589793;
const int SEG=12;
const int LENGTH_SEG=11;
vec3 center(float t,float time){
    float n=t+time*0.25;
    return vec3(sin(n*5.0)*0.58,cos(n*3.0)*0.48,cos(n*2.0)*0.36);
}
float radius(float t,float time){
    // Smooth deterministic radius replaces unavailable iChannel0 texture.
    return 0.18+0.065*sin(t*19.0+time*1.1)*sin(t*11.0-time*0.7);
}
vec3 surface(vec2 uv,float time){
    float t=uv.y;
    vec3 c=center(t,time);
    vec3 tangent=normalize(center(min(t+0.005,1.0),time)-
                           center(max(t-0.005,0.0),time));
    vec3 helper=abs(tangent.y)>0.85?vec3(1,0,0):vec3(0,1,0);
    vec3 right=normalize(cross(helper,tangent));
    vec3 up=normalize(cross(tangent,right));
    float a=uv.x*2.0*PI;
    float taper=smoothstep(0.0,0.12,t)*(1.0-smoothstep(0.88,1.0,t));
    float r=radius(t,time)*max(0.07,taper);
    float ripple=1.0+0.10*sin(t*75.0-time*4.0);
    return c+r*ripple*(right*cos(a)+up*sin(a));
}
void main(){
    int cell=gl_VertexIndex/6;
    int corner=gl_VertexIndex%6;
    int x=cell%SEG, y=cell/SEG;
    const ivec2 corners[6]=ivec2[6](
        ivec2(0,0),ivec2(1,0),ivec2(1,1),
        ivec2(0,0),ivec2(1,1),ivec2(0,1));
    vec2 uv=vec2(x+corners[corner].x,y+corners[corner].y)/
            vec2(SEG,LENGTH_SEG);
    float time=pc.parameters.x*max(pc.parameters.y,0.01);
    vec3 p=surface(uv,time);
    const float e=0.001;
    vec3 du=surface(uv+vec2(e,0),time)-surface(uv-vec2(e,0),time);
    vec3 dv=surface(clamp(uv+vec2(0,e),0.0,1.0),time)-
            surface(clamp(uv-vec2(0,e),0.0,1.0),time);
    vec3 n=normalize(cross(du,dv));
    vec3 radial=normalize(p-center(uv.y,time));
    if(dot(n,radial)<0.0)n=-n;
    vNormal=n;
    vPosition=p;
    vPulse=sin(uv.y*75.0-time*4.0);
    gl_Position=pc.mvp*vec4(p,1.0);
}
