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
    float theta=uv.x*2.0*PI, phi=uv.y*PI;
    vec3 d=vec3(sin(phi)*cos(theta),cos(phi),sin(phi)*sin(theta));
    float spikes=pow(max(0.0,sin(12.0*theta+0.8*t)*sin(9.0*phi-1.1*t)),5.0);
    float ripple=sin(17.0*d.x+2.0*t)*sin(15.0*d.y-t)*sin(16.0*d.z+1.3*t);
    float radius=0.64+0.05*sin(t*1.8)+amp*(0.90*spikes+0.16*ripple);
    return d*radius;
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
