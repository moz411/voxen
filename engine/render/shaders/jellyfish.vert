#version 450
// Procedural animated mesh; compatible with Organic Sphere fragment shader.
// 48x32 quads, 9216 vertices, no vertex buffers.
layout(push_constant) uniform PushConstants {
    mat4 mvp;
    vec4 parameters; // time, speed, hue, deformation
} pc;
layout(location=0) out vec3 vNormal;
layout(location=1) out vec3 vPosition;
layout(location=2) out float vPulse;
const float PI=3.141592653589793;
const int SEG=48;
const int RINGS=32;
vec3 surface(vec2 uv,float t,float amp){
    float theta=uv.x*2.0*PI;
    float phi=uv.y*PI;
    // Bell at the north pole, tapered trailing skirt below.
    float y=cos(phi);
    float radial=sin(phi)*(0.70+0.10*sin(t*2.2));
    float bell=smoothstep(-0.10,0.45,y);
    radial*=mix(0.34,1.0,bell);
    radial+=amp*0.18*sin(10.0*theta-2.5*t)*pow(1.0-bell,2.0);
    float sway=amp*0.18*sin(4.0*phi-1.8*t+2.0*theta)*(1.0-bell);
    return vec3(radial*cos(theta)+sway,0.72*y+0.12*sin(t*2.2)*bell,
                radial*sin(theta)+0.08*sin(t+theta)*(1.0-bell));
}
void main(){
    int cell=gl_VertexIndex/6;
    int corner=gl_VertexIndex%6;
    int x=cell%SEG, y=cell/SEG;
    const ivec2 offsets[6]=ivec2[6](
        ivec2(0,0),ivec2(1,0),ivec2(1,1),
        ivec2(0,0),ivec2(1,1),ivec2(0,1));
    vec2 uv=vec2(x+offsets[corner].x,y+offsets[corner].y)/vec2(SEG,RINGS);
    float t=pc.parameters.x*max(pc.parameters.y,0.01);
    float amp=clamp(pc.parameters.w,0.0,0.48);
    vec3 p=surface(uv,t,amp);
    const float e=0.001;
    vec3 du=surface(uv+vec2(e,0),t,amp)-surface(uv-vec2(e,0),t,amp);
    vec3 dv=surface(uv+vec2(0,e),t,amp)-surface(uv-vec2(0,e),t,amp);
    vec3 n=normalize(cross(du,dv));
    if(length(n)<0.5)n=normalize(p+vec3(0.001));
    vNormal=n;
    vPosition=p;
    vPulse=sin(12.0*uv.x+9.0*uv.y+2.0*t);
    gl_Position=pc.mvp*vec4(p,1.0);
}
