#version 450
// CLOUD Voxen: low-cost animated shell approximation of the supplied
// folded smooth-union cloud SDF. Vertex-only turbulence for Quest 3.
layout(push_constant) uniform PushConstants {mat4 mvp;vec4 parameters;} pc;
layout(location=0) out vec3 vNormal;
layout(location=1) out vec3 vPosition;
layout(location=2) out float vPulse;
const float PI=3.141592653589793;
const int SEG=48;
const int RINGS=32;
float softMin(float a,float b,float k){
    float h=max(k-abs(a-b),0.0)/k;
    return min(a,b)-h*h*k*0.25;
}
float cloudField(vec3 q,float t){
    mat3 m=mat3(2,-2,1,1,2,2,-2,-1,2)/3.0;
    vec3 Q=q*0.8,p=m*Q,u=Q+vec3(1.8);
    float phase=0.2*t,s=1.5,d=length(p)-1.0;
    for(int i=0;i<4;i++){
        vec3 w=sin(u+phase+1.9*sin(0.8*u.yzx+vec3(1,2.3,0.4)));
        float k=1.4/(0.8+0.1*sin(dot(u,vec3(0.7,-0.5,0.6))-phase));
        p=abs(p+0.12*w.yzx)-vec3(0.5);
        w*=0.1;
        p=m*(p+2.0/(1.0+dot(w,w))*cross(w,p))*k;
        s*=k;
        d=softMin(d,(length(p)-0.9)/s,0.3/s);
        u=mat3(0,0.8,0.6,-0.8,0.4,-0.5,-0.6,-0.5,0.6)*u*1.8+1.3;
        phase*=1.5;
    }
    return max(-softMin(-d,q.y+0.2,0.2),length(Q)-3.0);
}
vec3 cloudSurface(vec3 dir,float t){
    // Directional radial displacement from the original folded field.
    float a=cloudField(dir*1.4,t);
    float b=cloudField(dir*2.1,t);
    float puff=0.16*sin(dir.x*7.0+t*0.34)*sin(dir.z*6.0-t*0.25);
    float r=1.12+clamp(-a*0.45-b*0.12,-0.28,0.38)+puff;
    r*=1.0+0.04*sin(t*0.5);
    return dir*max(0.45,r)*vec3(1.28,0.78,1.05);
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
    vec3 p=cloudSurface(dir,t);
    vec3 tangent=normalize(cross(abs(dir.y)>0.95?vec3(1,0,0):vec3(0,1,0),dir));
    vec3 bitangent=normalize(cross(dir,tangent));
    const float e=0.003;
    vec3 da=cloudSurface(normalize(dir+e*tangent),t)-cloudSurface(normalize(dir-e*tangent),t);
    vec3 db=cloudSurface(normalize(dir+e*bitangent),t)-cloudSurface(normalize(dir-e*bitangent),t);
    vec3 n=normalize(cross(da,db));
    if(dot(n,dir)<0.0)n=-n;
    vNormal=n;
    vPosition=p;
    vPulse=cloudField(dir*1.6,t);
    gl_Position=pc.mvp*vec4(p,1.0);
}
