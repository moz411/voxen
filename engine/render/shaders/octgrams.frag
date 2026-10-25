#version 450
// OCTGRAMS: Vulkan/OpenXR adaptation of the user-supplied Shadertoy.
// Repeated rotating SDF box assemblies and luminous accumulation.
// Fewer iterations than the original 99 for Quest GPU performance.
layout(location=0) in vec2 uv;
layout(location=0) out vec4 outColor;
layout(push_constant) uniform PushConstants {mat4 mvp;vec4 parameters;} pc;
mat2 rot(float a){float c=cos(a),s=sin(a);return mat2(c,s,-s,c);}
float sdBox(vec3 p,vec3 b){
    vec3 q=abs(p)-b;
    return length(max(q,0.0))+min(max(q.x,max(q.y,q.z)),0.0);
}
float boxShape(vec3 p,float scale){
    p*=scale;
    float base=sdBox(p,vec3(0.4,0.4,0.1))/1.5;
    p.xy*=5.0;
    p.y-=3.5;
    p.xy=rot(0.75)*p.xy;
    return -base;
}
float boxSet(vec3 pos,float time){
    float a=sin(time*0.4)*2.5;
    float scale=2.0-abs(sin(time*0.4))*1.5;
    vec3 p=pos;
    p.y+=a;p.xy=rot(0.8)*p.xy;
    float d=boxShape(p,scale);
    p=pos;p.y-=a;p.xy=rot(0.8)*p.xy;
    d=max(d,boxShape(p,scale));
    p=pos;p.x+=a;p.xy=rot(0.8)*p.xy;
    d=max(d,boxShape(p,scale));
    p=pos;p.x-=a;p.xy=rot(0.8)*p.xy;
    d=max(d,boxShape(p,scale));
    p=pos;p.xy=rot(0.8)*p.xy;
    d=max(d,boxShape(p,0.5)*6.0);
    d=max(d,boxShape(pos,0.5)*6.0);
    return d;
}
void main(){
    float time=pc.parameters.x*max(pc.parameters.y,0.01);
    vec2 p=uv*2.0-1.0;
    vec3 ro=vec3(0.0,-0.2,time*4.0);
    vec3 ray=normalize(vec3(p,1.5));
    ray.xy=rot(sin(time*0.03)*5.0)*ray.xy;
    ray.yz=rot(sin(time*0.05)*0.2)*ray.yz;
    float t=0.1,acc=0.0;
    // 56 samples instead of 99: retain luminous repetition at lower cost.
    for(int i=0;i<56;i++){
        vec3 pos=ro+ray*t;
        pos=mod(pos-2.0,4.0)-2.0;
        float d=abs(boxSet(pos,time-float(i)*0.01));
        d=max(d,0.01);
        acc+=exp(-d*23.0);
        t+=d*0.55;
        if(t>80.0)break;
    }
    vec3 col=vec3(acc*0.02);
    col+=vec3(0.0,0.2*abs(sin(time)),0.5+sin(time)*0.2);
    float alpha=clamp(1.0-t*(0.02+0.02*sin(time)),0.0,1.0);
    outColor=vec4(clamp(col,0.0,1.0),alpha*clamp(pc.parameters.w,0.0,0.6));
}
