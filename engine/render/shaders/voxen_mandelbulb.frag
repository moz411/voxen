#version 450
// Mandelbulb stereo volume v2. Magenta indicates a miss, never an invisible discard.
// A cube proxy is rasterized in local space; both eyes trace from their own position.
layout(location=0) in vec3 localPosition;
layout(location=0) out vec4 outColor;
layout(push_constant) uniform PushConstants {
    mat4 mvp;
    vec4 parameters;  // time, speed, hue, density
    vec4 eyeLocal;
} pc;

float bulbDE(vec3 p){
    vec3 z=p;
    float dr=1.0;
    float radius=0.0;
    for(int i=0;i<7;i++){
        radius=length(z);
        if(radius>2.0)break;
        float safeR=max(radius,0.00001);
        float theta=acos(clamp(z.z/safeR,-1.0,1.0));
        float phi=atan(z.y,z.x);
        dr=8.0*pow(safeR,7.0)*dr+1.0;
        float zr=pow(safeR,8.0);
        theta*=8.0;phi*=8.0;
        z=zr*vec3(sin(theta)*cos(phi),sin(theta)*sin(phi),cos(theta))+p;
    }
    return 0.5*log(max(radius,0.00001))*radius/max(dr,0.00001);
}
vec3 surfaceNormal(vec3 p){
    const float e=0.006;
    return normalize(vec3(
        bulbDE(p+vec3(e,0,0))-bulbDE(p-vec3(e,0,0)),
        bulbDE(p+vec3(0,e,0))-bulbDE(p-vec3(0,e,0)),
        bulbDE(p+vec3(0,0,e))-bulbDE(p-vec3(0,0,e))));
}
void main(){
    // Use the interpolated cube face as an exact point on the viewing ray.
    vec3 ro=pc.eyeLocal.xyz;
    vec3 rd=normalize(localPosition-ro);
    // Ray against a bounding sphere; this avoids an unstable slab interval
    // (especially when a component of rd is nearly zero).
    float b=dot(ro,rd);
    float c=dot(ro,ro)-1.80*1.80;
    float disc=b*b-c;
    if(disc<0.0){
        outColor=vec4(1.0,0.0,0.7,1.0);
        return;
    }
    float entry=max(0.0,-b-sqrt(disc));
    float exit=-b+sqrt(disc);
    if(exit<=entry){
        outColor=vec4(1.0,0.0,0.7,1.0);
        return;
    }

    float t=pc.parameters.x*pc.parameters.y*0.20;
    mat2 rot=mat2(cos(t),sin(t),-sin(t),cos(t));
    float march=entry;
    vec3 hitPos=vec3(0.0);
    bool hit=false;
    for(int step=0;step<72;step++){
        vec3 p=ro+rd*march;
        vec3 q=p;
        q.xz=rot*q.xz;
        float d=bulbDE(q);
        if(d<0.008){
            hitPos=q;
            hit=true;
            break;
        }
        march+=clamp(d,0.004,0.18);
        if(march>exit)break;
    }
    if(!hit){
        // Deliberately visible: tells us that rasterization and stereo rays
        // work but the distance estimator still missed the Mandelbulb.
        outColor=vec4(0.85,0.0,0.55,1.0);
        return;
    }

    vec3 n=surfaceNormal(hitPos);
    float lighting=0.26+0.74*max(dot(n,normalize(vec3(-0.55,0.85,1.0))),0.0);
    vec3 tint=0.35+0.4*cos(6.2831853*(pc.parameters.z+vec3(0.0,0.24,0.49)));
    float bands=0.82+0.18*cos(12.0*length(hitPos)+6.0*hitPos.y);
    outColor=vec4(tint*lighting*bands+0.12*pow(max(n.z,0.0),12.0),1.0);
}
