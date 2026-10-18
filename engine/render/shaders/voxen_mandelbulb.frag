#version 450
// Mandelbulb: power-8 triplex iteration, following the mathematical
// construction documented at skytopia.com/project/fractal/2mandelbulb.html
layout(location=0) in vec2 uv;
layout(location=0) out vec4 outColor;
layout(push_constant) uniform PushConstants {mat4 mvp;vec4 parameters;} pc;
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
void main(){
    vec2 q=uv*1.22;
    if(dot(q,q)>1.8)discard;
    float t=pc.parameters.x*pc.parameters.y*0.18;
    mat2 r=mat2(cos(t),sin(t),-sin(t),cos(t));
    vec3 ro=vec3(q,2.45);
    vec3 rd=vec3(0.0,0.0,-1.0);
    float travel=0.0;
    bool hit=false;
    vec3 p;
    for(int i=0;i<32;++i){
        p=ro+rd*travel;
        vec3 model=p;
        model.xz=r*model.xz;
        float d=bulbDE(model);
        if(d<0.003){p=model;hit=true;break;}
        travel+=clamp(d,0.006,0.12);
        if(travel>4.5)break;
    }
    if(!hit)discard;
    vec3 n=bulbNormal(p);
    float light=0.25+0.75*max(dot(n,normalize(vec3(-0.5,0.8,1.4))),0.0);
    vec3 tint=0.36+0.35*cos(6.2831853*(pc.parameters.z+vec3(0.0,0.20,0.43)));
    float detail=0.75+0.25*cos(19.0*length(p)+p.y*12.0);
    outColor=vec4(tint*light*detail+0.18*pow(max(n.z,0.0),12.0),0.95);
}
