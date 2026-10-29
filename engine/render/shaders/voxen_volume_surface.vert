#version 450
// Procedural textured 3D shells. Mode in pc.parameters.z: 0=cube, 1=octahedron.
// The fragments are image-space effects reused as animated textures per face.
layout(push_constant) uniform PushConstants { mat4 mvp; vec4 parameters; } pc;
layout(location=0) out vec2 uv;
const vec3 cube[8]=vec3[8](
 vec3(-.5,-.5,-.5),vec3(.5,-.5,-.5),vec3(.5,.5,-.5),vec3(-.5,.5,-.5),
 vec3(-.5,-.5,.5),vec3(.5,-.5,.5),vec3(.5,.5,.5),vec3(-.5,.5,.5));
const int cubeIdx[36]=int[36](
 0,2,1,0,3,2, 4,5,6,4,6,7,
 0,1,5,0,5,4, 3,7,6,3,6,2,
 0,4,7,0,7,3, 1,2,6,1,6,5);
const vec3 oct[6]=vec3[6](
 vec3(0.,.72,0.),vec3(0.,-.72,0.),vec3(.72,0.,0.),
 vec3(-.72,0.,0.),vec3(0.,0.,.72),vec3(0.,0.,-.72));
const int octIdx[24]=int[24](
 0,4,2,0,3,4,0,5,3,0,2,5,
 1,2,4,1,4,3,1,3,5,1,5,2);
void main(){
 int idx=gl_VertexIndex;
 if(pc.parameters.z<.5){
    vec3 p=cube[cubeIdx[idx]];
    int tri=idx/3;
    // Per face: a coherent texture basis, rather than orthogonal projections.
    vec3 a=cube[cubeIdx[(tri/2)*6]], b=cube[cubeIdx[(tri/2)*6+1]], c=cube[cubeIdx[(tri/2)*6+2]];
    vec3 u=normalize(b-a), v=normalize(c-a-u*dot(c-a,u));
    vec3 midpoint=(a+b+c)/3.;
    uv=vec2(dot(p-midpoint,u),dot(p-midpoint,v))*1.7;
    gl_Position=pc.mvp*vec4(p,1.);
 }else{
    int face=idx/3;
    vec3 a=oct[octIdx[face*3]],b=oct[octIdx[face*3+1]],c=oct[octIdx[face*3+2]];
    vec3 p=idx%3==0?a:idx%3==1?b:c;
    vec3 u=normalize(b-a),v=normalize(c-a-u*dot(c-a,u));
    uv=vec2(dot(p-(a+b+c)/3.,u),dot(p-(a+b+c)/3.,v))*1.3;
    gl_Position=pc.mvp*vec4(p,1.);
 }
}
