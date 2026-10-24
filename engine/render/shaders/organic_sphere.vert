#version 450
// Organic Sphere demo. Procedural UV sphere, no vertex buffer.
// Draw 48 * 32 * 6 = 9216 vertices (3072 triangles).
layout(push_constant) uniform PushConstants {
    mat4 mvp;
    vec4 parameters; // time, speed, hue, amplitude
} pc;
layout(location=0) out vec3 vNormal;
layout(location=1) out vec3 vPosition;
layout(location=2) out float vPulse;
const float PI=3.141592653589793;
const int SEG=48;
const int RINGS=32;
float field(vec3 p,float t){
    float a=sin(7.0*p.x+2.1*t)*cos(6.0*p.y-1.3*t)*sin(8.0*p.z+0.9*t);
    float b=sin(13.0*p.y+4.0*p.z-1.7*t)*sin(11.0*p.x-3.0*p.z+1.1*t);
    float c=sin(3.0*(p.x+p.y+p.z)+0.65*t);
    return 0.58*a+0.24*b+0.18*c;
}
vec3 surface(vec3 direction,float t,float amp){
    float f=field(direction,t);
    float breathe=0.045*sin(t*1.4);
    return direction*(1.0+breathe+amp*f);
}
void main(){
    int cell=gl_VertexIndex/6;
    int corner=gl_VertexIndex%6;
    int x=cell%SEG, y=cell/SEG;
    const ivec2 offsets[6]=ivec2[6](
        ivec2(0,0),ivec2(1,0),ivec2(1,1),
        ivec2(0,0),ivec2(1,1),ivec2(0,1));
    vec2 uv=vec2(x+offsets[corner].x,y+offsets[corner].y)/vec2(SEG,RINGS);
    float theta=uv.x*2.0*PI;
    float phi=uv.y*PI;
    vec3 d=vec3(sin(phi)*cos(theta),cos(phi),sin(phi)*sin(theta));
    float t=pc.parameters.x*max(pc.parameters.y,0.01);
    float amp=clamp(pc.parameters.w,0.0,0.48);
    // Bubble must remain spherical rather than deforming like organic presets.
    if(int(floor(pc.parameters.z+0.0001))==4)amp=0.0;
    vec3 p=surface(d,t,amp);
    // Analytic finite-difference tangent normals, independent of mesh density.
    vec3 tangent=normalize(cross(abs(d.y)>0.95?vec3(1,0,0):vec3(0,1,0),d));
    vec3 bitangent=normalize(cross(d,tangent));
    const float e=0.003;
    vec3 a=surface(normalize(d+e*tangent),t,amp)-surface(normalize(d-e*tangent),t,amp);
    vec3 b=surface(normalize(d+e*bitangent),t,amp)-surface(normalize(d-e*bitangent),t,amp);
    vec3 n=normalize(cross(a,b));
    if(dot(n,d)<0.0)n=-n;
    vNormal=n;
    vPosition=p;
    vPulse=field(d,t);
    gl_Position=pc.mvp*vec4(p,1.0);
}
