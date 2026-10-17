#version 450
// Adaptation of user-supplied Shadertoy MddXzl fragment.
// Rendering: localized procedural raymarch on a Vulkan quad.
// Push constants: x=time seconds, y=animation speed, z=scale, w=mode.
layout(location=0) in vec2 uv;
layout(location=0) out vec4 outColor;
layout(push_constant) uniform PushConstants { mat4 mvp; vec4 parameters; } pc;
const vec4 HASH_SCALE=vec4(1031.0,0.1030,0.0973,0.1099);
vec4 hash43(vec3 p) {
    vec4 p4=fract(vec4(p.xyzx)*HASH_SCALE);
    p4+=dot(p4,p4.wzxy+19.19);
    return fract(vec4((p4.x+p4.y)*p4.z,(p4.x+p4.z)*p4.y,
                      (p4.y+p4.z)*p4.w,(p4.z+p4.w)*p4.x));
}
vec4 sphereAt(vec3 p,float time) {
    vec4 s=hash43(p)+vec4(p,0.0);
    s.w*=smoothstep(1.0,8.0,distance(s.xyz,vec3(0.0,0.0,time*0.5)));
    return s;
}
float field(vec3 p,float time) {
    vec3 tile=floor(p);
    float distanceToSurface=1.0;
    for(int z=-1;z<=1;++z)
    for(int y=-1;y<=1;++y)
    for(int x=-1;x<=1;++x) {
        vec4 s=sphereAt(tile+vec3(x,y,z),time);
        if(s.w>0.0) distanceToSurface=min(distanceToSurface,length(p-s.xyz)-s.w);
    }
    return distanceToSurface;
}
vec3 normalAt(vec3 p,float time) {
    const float delta=0.01;
    float c=field(p,time);
    vec3 g=vec3(field(p+vec3(delta,0,0),time)-c,
                field(p+vec3(0,delta,0),time)-c,
                field(p+vec3(0,0,delta),time)-c);
    return normalize(g+vec3(1e-7));
}
void main() {
    float time=pc.parameters.x*pc.parameters.y;
    vec3 origin=vec3(sin(time),0.0,time*0.5);
    vec3 direction=normalize(vec3(uv*vec2(1.0,1.0),0.7));
    vec3 point=origin;
    bool hit=false;
    // The source is expensive (27 cell checks for each field() call).
    // Fixed maximum loop, bounded travel distance, no extrapolation x10.
    for(int step=0;step<8;++step) {
        float d=field(point,time);
        if(d<0.003) { hit=true; break; }
        point+=direction*clamp(d,0.008,0.5);
        if(length(point-origin)>5.0) break;
    }
    if(!hit) discard;
    vec3 n=normalAt(point,time);
    float light=max(0.0,dot(n,normalize(vec3(-0.5,1.0,-2.0))));
    float gray=pow(clamp(0.14+0.86*light,0.0,1.0),1.0/2.2);
    // Limited luminosity/opacity; preserve real-world perception.
    outColor=vec4(vec3(gray)*vec3(0.30,0.85,0.98),clamp(pc.parameters.w,0.0,0.6));
}
