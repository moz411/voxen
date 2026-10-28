#version 450
// VORONOI FALL — standalone reduced interpretation of user-supplied
// multi-pass falling polygon / log-polar Voronoi shader.
// The original requires Common definitions and a Buffer A containing
// piece geometry, rotations and state; this is NOT an exact port.
layout(location=0) in vec2 uv;
layout(location=0) out vec4 outColor;
layout(push_constant) uniform PushConstants { mat4 mvp; vec4 parameters; } pc;
float hash21(vec2 p){
    vec3 q=fract(vec3(p.x,p.y,p.x)*vec3(.1031,.1030,.0973));
    q+=dot(q,q.yzx+33.33);
    return fract((q.x+q.y)*q.z);
}
vec2 rotate2(vec2 p,float a){
    float c=cos(a),s=sin(a);
    return mat2(c,s,-s,c)*p;
}
// Sample nearest and second-nearest Voronoi sites in a log-polar strip.
vec3 cellColor(vec2 x,float t,out float edge){
    float r=max(length(x),.0003);
    float angle=atan(x.y,x.x);
    vec2 warped=vec2(log(r)*2.6-t*.23,angle*2.7-log(r)*.34);
    vec2 base=floor(warped);
    float best=1e6,second=1e6;
    vec2 winner=base;
    vec2 bestSite=vec2(0.);
    for(int j=-2;j<=2;j++)for(int i=-2;i<=2;i++){
        vec2 id=base+vec2(i,j);
        float h=hash21(id);
        vec2 site=id+vec2(.5)+.31*(vec2(hash21(id+13.4),hash21(id+81.6))-.5);
        float d=length(warped-site);
        if(d<best){second=best;best=d;winner=id;bestSite=site;}
        else if(d<second)second=d;
    }
    float gap=second-best;
    edge=1.-smoothstep(.025,.085,gap);
    float seed=hash21(winner+4.9);
    vec3 material=mix(vec3(.58,.54,.48),vec3(.92,.82,.67),seed);
    float shade=.7+.3*dot(normalize(vec3(warped-bestSite,.6)),normalize(vec3(.9,.1,.5)));
    return material*shade;
}
float polygon(vec2 p,float sides,float radius,float rotation){
    float a=atan(p.y,p.x)+rotation;
    float sector=6.2831853/sides;
    return cos(floor(.5+a/sector)*sector-a)*length(p)-radius;
}
void main(){
    float time=pc.parameters.x*pc.parameters.y;
    vec2 p=uv;
    float edge;
    vec3 col=cellColor(p,time,edge);
    float hole=1.-smoothstep(.045,.085,length(p));
    col*=1.-edge*.95;
    col*=1.-hole;
    // Flying plates tumble as they approach the central opening.
    for(int k=0;k<7;k++){
        float fi=float(k);
        float phase=fract(time*.14+fi*.161);
        float radius=mix(1.18,.05,phase);
        float angle=fi*2.39996+time*.12;
        vec2 center=vec2(cos(angle),sin(angle))*radius;
        vec2 local=rotate2(p-center,time*(.55+fi*.07));
        float sides=3.+mod(fi,4.);
        float d=polygon(local,sides,mix(.14,.018,phase),fi);
        float face=1.-smoothstep(-.007,.008,d);
        float bevel=1.-smoothstep(.003,.027,abs(d));
        float depth=mix(.45,1.0,phase);
        vec3 plate=mix(vec3(.31,.38,.48),vec3(1.,.87,.69),hash21(vec2(fi,2.)));
        vec2 lightDirection=normalize(vec2(.9,.35));
        float sideLight=.55+.45*dot(normalize(local+vec2(.001)),lightDirection);
        plate*=depth*(.8+.2*sideLight);
        col=mix(col,plate,face);
        col+=plate*bevel*.12;
    }
    col=pow(clamp(col,0.,1.),vec3(.96));
    outColor=vec4(col,pc.parameters.w);
}
