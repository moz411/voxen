#version 450
// Voxel brush geometry; no CPU meshes or asset imports.
// shape: 0 cube, 1 sphere, 2 cone, 3 torus, 4 cylinder,
// 5 star, 6 cross, 7 diamond.
layout(push_constant) uniform PushConstants {mat4 mvp;vec4 parameters;} pc;
layout(location=0) out vec3 tint;
const float PI=3.141592653589793;
const int S=32;
const int R=16;
const vec3 corners[8]=vec3[](
    vec3(-0.5,-0.5,-0.5),vec3(0.5,-0.5,-0.5),vec3(0.5,0.5,-0.5),vec3(-0.5,0.5,-0.5),
    vec3(-0.5,-0.5,0.5),vec3(0.5,-0.5,0.5),vec3(0.5,0.5,0.5),vec3(-0.5,0.5,0.5));
const int cubeIndices[36]=int[](
    0,2,1,0,3,2,4,5,6,4,6,7,0,1,5,0,5,4,
    3,7,6,3,6,2,0,4,7,0,7,3,1,2,6,1,6,5);
vec3 parametric(int shape,vec2 uv){
    float a=uv.x*2.0*PI;
    float b=uv.y*PI;
    if(shape==1)return 0.5*vec3(sin(b)*cos(a),cos(b),sin(b)*sin(a));
    if(shape==3){
        float t=uv.y*2.0*PI;
        return vec3((0.33+0.13*cos(t))*cos(a),0.13*sin(t),(0.33+0.13*cos(t))*sin(a));
    }
    if(shape==2)return vec3(0.5*(1.0-uv.y)*cos(a),uv.y-0.5,0.5*(1.0-uv.y)*sin(a));
    if(shape==4)return vec3(0.5*cos(a),uv.y-0.5,0.5*sin(a));
    if(shape==5){
        float r=0.26+0.24*pow(abs(cos(a*5.0)),12.0);
        return vec3(r*sin(b)*cos(a),0.5*cos(b),r*sin(b)*sin(a));
    }
    if(shape==6){
        vec3 p=vec3(sin(b)*cos(a),cos(b),sin(b)*sin(a));
        float reach=0.18+0.32*pow(max(max(abs(p.x),abs(p.y)),abs(p.z)),10.0);
        return p*reach;
    }
    // Diamond / octahedron profile
    float r=0.5*(1.0-abs(uv.y*2.0-1.0));
    return vec3(r*cos(a),uv.y-0.5,r*sin(a));
}
void main(){
    int shape=clamp(int(pc.parameters.w+0.5),0,7);
    vec3 p;
    if(shape==0)p=corners[cubeIndices[gl_VertexIndex]];
    else {
        int cell=gl_VertexIndex/6;
        int corner=gl_VertexIndex%6;
        int x=cell%S,y=cell/S;
        const ivec2 offsets[6]=ivec2[](ivec2(0,0),ivec2(1,0),ivec2(1,1),ivec2(0,0),ivec2(1,1),ivec2(0,1));
        vec2 uv=vec2(x+offsets[corner].x,y+offsets[corner].y)/vec2(S,R);
        p=parametric(shape,uv);
    }
    tint=clamp(pc.parameters.xyz,0.0,1.0);
    gl_Position=pc.mvp*vec4(p,1.0);
}
