#version 450
#define VOXEN_INSTANCED
layout(location=0) in vec3 localColor;
layout(location=0) out vec4 outColor;
#ifdef VOXEN_INSTANCED
layout(location=1) in vec2 instanceStyle;
#endif
layout(push_constant) uniform Push {
    mat4 mvp;
    vec4 parameters; // x time, y speed, z hue [0,1], w material ID
} pc;
vec2 rot(vec2 p,float a){float s=sin(a),c=cos(a);return mat2(c,-s,s,c)*p;}
float hash(vec2 p){return fract(sin(dot(p,vec2(127.1,311.7)))*43758.5453);}
float noise2(vec2 p){
    vec2 i=floor(p),f=fract(p); f=f*f*(3.-2.*f);
    return mix(mix(hash(i),hash(i+vec2(1,0)),f.x),
               mix(hash(i+vec2(0,1)),hash(i+vec2(1,1)),f.x),f.y);
}
float voronoi(vec2 p){
    vec2 g=floor(p),f=fract(p);float d=10.;
    for(int y=-1;y<=1;y++)for(int x=-1;x<=1;x++){
        vec2 o=vec2(x,y),h=vec2(hash(g+o),hash(g+o+23.));
        d=min(d,length(o+0.5+0.4*sin(6.283*h)-f));
    }
    return d;
}
vec3 palette(float h){return 0.5+0.5*cos(6.283*(h+vec3(0.,0.3333,0.6667)));}
void main(){
    vec3 p=localColor*2.-1.;
    float t=pc.parameters.x*pc.parameters.y;
    float id=mod(pc.parameters.w,100.0);
#ifdef VOXEN_INSTANCED
    id=instanceStyle.y;
#endif
    bool wall=pc.parameters.w>=100.0;
    float hue=pc.parameters.z;
#ifdef VOXEN_INSTANCED
    hue=instanceStyle.x;
#endif
    vec3 c=palette(hue);
    if(id<0.5){} // MATTE
    else if(id<1.5){
        vec3 g=abs(fract(p*9.+0.5)-0.5);
        c*=0.5+1.5*(1.-smoothstep(.03,.1,min(g.x,min(g.y,g.z))));
    }else if(id<2.5){
        float v=sin(p.x*16.+t*2.)+sin(p.y*19.-t*3.)+sin(p.z*13.+t*1.6);
        c=0.55+0.45*cos(v*2.1+vec3(0.,2.1,4.2));
    }else if(id<3.5){c=vec3(.08,.7,1.)*(.55+.45*sin(p.y*110.-t*11.));}
    else if(id<4.5){float v=sin(p.x*16.+sin(p.y*13.+t*2.5))+sin(p.z*17.-t*1.7);c=vec3(1.,.15+.32*clamp(v+.5,0.,1.),.015);}
    else if(id<5.5){float v=pow(abs(sin((p.x+p.z)*32.+sin(p.y*19.+t))),12.);c=vec3(.2,.68,1.)*(.7+.3*v);}
    else if(id<6.5){
        vec2 q=rot(p.xy*3.,t*5.);q=rot(q,-5.*length(q));
        float v=pow(voronoi(q*1.3),2.5);
        float ring=1.-smoothstep(.52,.7,length(p.xy));
        c=(vec3(.03,.2,.9)+v*vec3(.2,.65,1.))*ring;
    }else if(id<7.5){
        vec2 q=p.xy*12.;float v=0.;
        for(int i=0;i<10;i++){
            float f=float(i)/10.,x=mod(t*(.5+f)+f*13.,12.)-6.;
            v+=exp(-25.*dot(q-vec2(x,-6.+12.*f),q-vec2(x,-6.+12.*f)));
        }
        c=palette(hue+v*.15)*clamp(v,0.,1.);
    }else{
        float a=0.5,w=0.,v=0.;vec2 q=p.xy*9.-vec2(t,0.);
        for(int i=0;i<6;i++){v+=noise2(q)*a;w+=a;q*=2.;a*=.5;}
        c=palette(hue)*(v/w);
    }
    outColor=vec4(clamp(c,0.,1.),wall?0.38:1.);
}
