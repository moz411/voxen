#version 450
// Adapted for Vulkan/OpenXR from Hepp Maccoy (2019), hepp@audiopixel.com
// http://audiopixel.com
// Distance functions credited to Inigo Quilez, https://iquilezles.org
// Original code licensed CC BY-NC-SA 3.0 Unported.
// Adaptation is distributed under the same license; attribution retained.
// Quest fast path reduces original 50 raymarch steps to 20.
layout(location=0) in vec2 uv;
layout(location=0) out vec4 outColor;
layout(push_constant) uniform PushConstants { mat4 mvp; vec4 parameters; } pc;
#define PI 3.14159265358979323846
vec3 glow;
float d1;
float t1;
vec3 b_8(vec3 c1,vec3 c2){return c1+c2-c1*c2;}
vec3 b_6(vec3 c1,vec3 c2){return c1+c2-1.0;}
float soc(vec3 p){vec3 n=normalize(sign(p+1e6));return min(min(dot(p.xy,n.xy),dot(p.yz,n.yz)),dot(p.xz,n.xz));}
float sinc(float x,float k){float a=PI*(k*x-1.0);return sin(a)/max(abs(a),0.00001)*sign(a);}
mat2 r2d(float a){float sa=sin(a),ca=cos(a);return mat2(ca,sa,-sa,ca);}
vec2 mo(vec2 p,vec2 d){vec2 q=abs(p)-d;if(q.y>q.x)q=q.yx;return q;}
float mapScene(vec3 p){
    float d=0.328;
    float a=abs(p.y);
    p.yz*=r2d(sign(a)*5.6);
    p.xz*=r2d(sign(a)*d1*4.2);
    p.xz=mo(p.xz,vec2((-d1*29.8)-1.,-7.3676));
    p.zx=mo(p.xz,vec2(8.4443,4.0344));
    p.xz=mo(p.xz,vec2(-12.9879,-9.1065));
    p.zx=mo(p.zx,vec2(5.4177,2.3984));
    p.xz=max(abs(p.xz)+8.7279,10.023);
    p.xz=mo(p.xz,vec2((d1*2.)-4.,-12.7982));
    p.zx=mo(p.xz,vec2(-16.7502,-8.5611));
    p.z=mod(p.z,-16.381445)+8.1907225;
    p.x=mod(p.x,76.08105)-38.040525;
    p.y=mod(p.y+t1*21.2,28.46435)-5.;
    p.zy=mo(p.zy,vec2(0.,d1*2.8));
    p.xz=mo(p.xz,vec2(0.,14.1212));
    p.yx=mo(p.yx,vec2(2.3815,0.5875));
    d=min(d,soc(max(abs(p)-1.3103,-0.9418)));
    glow+=vec3(0.8784314)*0.0471/(0.0539+d*d);
    return (length(p*-0.6424)+0.7392)*0.1944+d*1.939;
}
void main(){
    t1=(pc.parameters.x*pc.parameters.y*0.78)+166.;
    d1=sin(t1*0.2);
    glow=vec3(0.);
    // Wall quad UV in [-1,1], mapped to the original Shadertoy aspect ratio.
    vec2 st=(uv*0.5+0.5)*2.1-1.;
    vec3 ro=vec3(st,3.);
    vec3 rd=normalize(vec3(st,-0.35938));
    vec3 mp=ro;
    for(int i=0;i<20;i++){
        float md=mapScene(mp);
        mp+=rd*0.5921*md;
    }
    float b=length(ro-mp);
    float dA=1.6979+b*0.1462*0.6102;
    float dB=0.9117+b*0.1348*1.998;
    dA=sinc(dA,0.3252);
    dB=sinc(dB,0.2127);
    float src1=dA*-0.3094*2.;
    float src2=dB*-1.*0.9038;
    float src3=dB*-1.;
    float src4=dA*0.4847;
    vec3 c=b_6(mix(vec3(0.,0.5882353,1.),vec3(0.),src1),
               mix(vec3(0.),vec3(1.,0.,0.9411765),src2)*1.211);
    c*=src3;
    c=b_6(c,vec3(src4));
    vec3 gt=c+glow*0.1;
    c=b_8(c,gt);
    outColor=vec4(clamp(c,0.,1.),clamp(pc.parameters.w,0.,0.6));
}
