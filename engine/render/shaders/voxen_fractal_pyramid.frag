#version 450
// Fractal Pyramid: Vulkan port of the user-provided Shadertoy raymarch.
// A bounded, transparent camera-facing local quad, no external textures.
layout(location=0) in vec2 uv;
layout(location=0) out vec4 outColor;
layout(push_constant) uniform PushConstants { mat4 mvp; vec4 parameters; } pc;
mat2 rotate2(float a){float c=cos(a),s=sin(a);return mat2(c,s,-s,c);}
vec3 palette(float d){return mix(vec3(0.2,0.7,0.9),vec3(1.0,0.0,1.0),clamp(d,0.0,1.0));}
float mapPyramid(vec3 p,float time){
    for(int i=0;i<8;++i){
        float t=time*0.2;
        p.xz=rotate2(t)*p.xz;
        p.xy=rotate2(t*1.89)*p.xy;
        p.xz=abs(p.xz)-vec2(0.5);
    }
    return dot(sign(p),p)/5.0;
}
void main(){
    vec2 q=uv;
    if(dot(q,q)>1.9)discard;
    float time=pc.parameters.x*pc.parameters.y;
    // Original Shadertoy ro=-50 is too distant for a small physical object;
    // use a local camera orbit to keep the structure legible at Quest scale.
    vec3 ro=vec3(0.0,0.0,-3.5);
    ro.xz=rotate2(time*0.35)*ro.xz;
    vec3 forward=normalize(-ro);
    vec3 side=normalize(cross(forward,vec3(0.0,1.0,0.0)));
    vec3 up=normalize(cross(forward,side));
    vec3 rd=normalize(forward*2.2+q.x*side+q.y*up);
    vec3 accum=vec3(0.0);
    float travel=0.0;
    float alpha=0.0;
    for(int i=0;i<48;++i){
        vec3 p=ro+rd*travel;
        float d=mapPyramid(p,time)*0.5;
        if(!isnan(d) && !isinf(d) && d<0.012){alpha=1.0;break;}
        if(isnan(d)||isinf(d)||d>50.0)break;
        float stepSize=clamp(abs(d),0.012,0.22);
        accum+=palette(length(p)*0.1)/(400.0*stepSize);
        travel+=stepSize;
        if(travel>7.0)break;
    }
    vec3 color=1.0-exp(-accum*2.4);
    float glow=clamp(max(max(color.r,color.g),color.b)*1.2,0.0,0.9);
    alpha=max(alpha*0.92,glow);
    alpha*=1.0-smoothstep(0.85,1.35,length(q));
    if(alpha<0.015)discard;
    outColor=vec4(color,alpha);
}
