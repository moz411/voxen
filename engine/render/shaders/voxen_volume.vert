#version 450
// Experimental Mandelbulb surface mesh: real 3D triangles, no varyings.
// Explicit geometry from gl_VertexIndex, fixed-color fragment stage.
// This deliberately avoids the varying/fragment-ray path which went invisible.
layout(push_constant) uniform PushConstants {
    mat4 mvp;
    vec4 parameters;
} pc;
const int SLICES = 32;
const int STACKS = 20;
const float PI = 3.14159265358979323846;
float bulbDistance(vec3 p) {
    vec3 z=p;
    float dr=1.0;
    float r=0.0;
    for (int i=0;i<5;i++) {
        r=length(z);
        if(r>2.0) break;
        float safeR=max(r,0.0001);
        float theta=acos(clamp(z.z/safeR,-1.0,1.0))*8.0;
        float phi=atan(z.y,z.x)*8.0;
        dr=8.0*pow(safeR,7.0)*dr+1.0;
        float zr=pow(safeR,8.0);
        z=zr*vec3(sin(theta)*cos(phi),sin(theta)*sin(phi),cos(theta))+p;
    }
    return 0.5*log(max(r,0.0001))*r/max(dr,0.0001);
}
vec3 surfaceVertex(int sx,int sy){
    float lon=2.0*PI*float(sx)/float(SLICES);
    float lat=PI*float(sy)/float(STACKS);
    vec3 dir=vec3(sin(lat)*cos(lon),cos(lat),sin(lat)*sin(lon));
    // March from outside towards the center; stop at the DE surface.
    float radius=1.55;
    for(int i=0;i<20;i++){
        float d=bulbDistance(dir*radius);
        if(d<0.008)break;
        radius=max(0.03,radius-clamp(d,0.009,0.16));
    }
    return dir*radius*0.47;
}
void main(){
    int tri=gl_VertexIndex/3;
    int corner=gl_VertexIndex%3;
    int cell=tri/2;
    int sx=cell%SLICES;
    int sy=cell/SLICES;
    int ox=0,oy=0;
    if(tri%2==0) {
        if(corner==1)ox=1;
        if(corner==2){ox=1;oy=1;}
    } else {
        if(corner==0){ox=1;oy=1;}
        if(corner==1)oy=1;
    }
    vec3 p=surfaceVertex(sx+ox,sy+oy);
    gl_Position=pc.mvp*vec4(p,1.0);
}
