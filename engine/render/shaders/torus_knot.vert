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
    float a=uv.x*2.0*PI;
    float b=uv.y*2.0*PI;
    float r=0.56+0.22*cos(3.0*a);
    vec3 center=vec3(r*cos(2.0*a),r*sin(2.0*a),0.24*sin(3.0*a));
    // Stable tube frame from curve derivative, including periodic seam.
    float h=0.001;
    float ar=a+h, al=a-h;
    vec3 cr=vec3((0.56+0.22*cos(3.0*ar))*cos(2.0*ar),
                  (0.56+0.22*cos(3.0*ar))*sin(2.0*ar),0.24*sin(3.0*ar));
    vec3 cl=vec3((0.56+0.22*cos(3.0*al))*cos(2.0*al),
                  (0.56+0.22*cos(3.0*al))*sin(2.0*al),0.24*sin(3.0*al));
    vec3 tangent=normalize(cr-cl);
    vec3 axis=normalize(cross(tangent,vec3(0.0,0.0,1.0)));
    vec3 side=normalize(cross(tangent,axis));
    float tube=0.18*(1.0+amp*0.5*sin(8.0*a-2.0*t+4.0*b));
    return center+tube*(cos(b+t*0.15)*axis+sin(b+t*0.15)*side);
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
