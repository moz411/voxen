#version 450
// Mandelbulb: power-8 triplex iteration, following the mathematical
// construction documented at skytopia.com/project/fractal/2mandelbulb.html
layout(location=0) in vec3 localPosition;
layout(location=0) out vec4 outColor;
layout(push_constant) uniform PushConstants {mat4 mvp;vec4 parameters;vec4 eyeLocal;} pc;
float bulbDE(vec3 p){
    vec3 z=p;
    float dr=1.0,r=0.0;
    const float power=8.0;
    for(int i=0;i<6;++i){
        r=length(z);
        if(r>2.0)break;
        float theta=acos(clamp(z.z/max(r,0.0001),-1.0,1.0));
        float phi=atan(z.y,z.x);
        dr=power*pow(max(r,0.0001),power-1.0)*dr+1.0;
        float zr=pow(r,power);
        theta*=power;
        phi*=power;
        z=zr*vec3(sin(theta)*cos(phi),sin(theta)*sin(phi),cos(theta))+p;
    }
    return 0.5*log(max(r,0.0001))*r/max(dr,0.0001);
}
vec3 bulbNormal(vec3 p){
    const float h=0.006;
    return normalize(vec3(
      bulbDE(p+vec3(h,0,0))-bulbDE(p-vec3(h,0,0)),
      bulbDE(p+vec3(0,h,0))-bulbDE(p-vec3(0,h,0)),
      bulbDE(p+vec3(0,0,h))-bulbDE(p-vec3(0,0,h))));
}

// Cube segment in object-local coordinates. Use the real eye position for each eye.
vec2 cubeInterval(vec3 ro,vec3 rd){
    vec3 safeRd=sign(rd)*max(abs(rd),vec3(0.00001));
    vec3 a=(-vec3(1.0)-ro)/safeRd;
    vec3 b=(vec3(1.0)-ro)/safeRd;
    vec3 lo=min(a,b),hi=max(a,b);
    return vec2(max(max(lo.x,lo.y),lo.z),min(min(hi.x,hi.y),hi.z));
}
void main(){
    vec3 ro=pc.eyeLocal.xyz;
    vec3 rd=normalize(localPosition-ro);
    vec2 segment=cubeInterval(ro,rd);
    float travel=max(segment.x,0.0);
    float endTravel=segment.y;
    if(endTravel<=travel)discard;
    float t=pc.parameters.x*pc.parameters.y*0.18;
    mat2 r=mat2(cos(t),sin(t),-sin(t),cos(t));
    vec3 hitPoint;
    bool hit=false;
    for(int i=0;i<36;++i){
        vec3 p=ro+rd*travel;
        vec3 model=p;
        model.xz=r*model.xz;
        float d=bulbDE(model);
        if(d<0.004){hitPoint=p;hit=true;break;}
        travel+=clamp(d,0.007,0.12);
        if(travel>endTravel)break;
    }
    if(!hit)discard;
    vec3 modelHit=hitPoint;
    modelHit.xz=r*modelHit.xz;
    vec3 n=bulbNormal(modelHit);
    float light=0.23+0.77*max(dot(n,normalize(vec3(-0.5,0.8,1.4))),0.0);
    vec3 tint=0.36+0.35*cos(6.2831853*(pc.parameters.z+vec3(0.0,0.20,0.43)));
    float detail=0.75+0.25*cos(19.0*length(modelHit)+modelHit.y*12.0);
    outColor=vec4(tint*light*detail+0.18*pow(max(n.z,0.0),12.0),1.0);
    vec4 clip=pc.mvp*vec4(hitPoint,1.0);
    gl_FragDepth=clamp(clip.z/clip.w,0.0,1.0);
}
