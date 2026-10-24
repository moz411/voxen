#version 450
// METABALLS Voxen: animated 16-sphere smooth union adapted from user GLSL.
// Mesh extraction uses a bounded radial solve, not per-pixel raymarching.
layout(push_constant) uniform PushConstants {mat4 mvp;vec4 parameters;} pc;
layout(location=0) out vec3 vNormal;
layout(location=1) out vec3 vPosition;
layout(location=2) out float vPulse;
const float PI=3.14159265359;
const int SEG=32;
const int RINGS=24;
float smoothUnion(float d1,float d2,float k){
    float h=clamp(0.5+0.5*(d2-d1)/k,0.0,1.0);
    return mix(d2,d1,h)-k*h*(1.0-h);
}
float field(vec3 p,float t){
    float d=2.0;
    for(int i=0;i<16;i++){
        float fi=float(i);
        float seed=fract(fi*412.531+0.513);
        float time=t*(seed-0.5)*2.0;
        vec3 center=sin(time+fi*vec3(52.5126,64.62744,632.25))
                    *vec3(0.86,0.86,0.34);
        float radius=mix(0.5,1.0,fract(fi*412.531+0.5124))*0.43;
        d=smoothUnion(length(p+center)-radius,d,0.23);
    }
    // Central core guarantees one connected radial surface.
    return smoothUnion(d,length(p)-0.37,0.28);
}
vec3 surface(vec3 dir,float t){
    // Find an approximate outer isosurface along each radial direction.
    float lo=0.0,hi=1.7;
    for(int j=0;j<6;j++){
        float mid=0.5*(lo+hi);
        if(field(dir*mid,t)<0.0)lo=mid;else hi=mid;
    }
    return dir*max(0.08,0.5*(lo+hi));
}
void main(){
    int cell=gl_VertexIndex/6,corner=gl_VertexIndex%6;
    int x=cell%SEG,y=cell/SEG;
    const ivec2 corners[6]=ivec2[6](
        ivec2(0,0),ivec2(1,0),ivec2(1,1),
        ivec2(0,0),ivec2(1,1),ivec2(0,1));
    vec2 uv=vec2(x+corners[corner].x,y+corners[corner].y)/vec2(SEG,RINGS);
    float theta=uv.x*2.0*PI,phi=uv.y*PI;
    vec3 dir=vec3(sin(phi)*cos(theta),cos(phi),sin(phi)*sin(theta));
    float t=pc.parameters.x*max(pc.parameters.y,0.01);
    vec3 p=surface(dir,t);
    // Field-gradient normal at the extracted surface.
    const float e=0.012;
    vec3 n=normalize(vec3(
        field(p+vec3(e,0,0),t)-field(p-vec3(e,0,0),t),
        field(p+vec3(0,e,0),t)-field(p-vec3(0,e,0),t),
        field(p+vec3(0,0,e),t)-field(p-vec3(0,0,e),t)));
    vNormal=n;
    vPosition=p;
    vPulse=sin(t*3.0+dot(p,vec3(2.0,3.0,4.0)));
    gl_Position=pc.mvp*vec4(p,1.0);
}
