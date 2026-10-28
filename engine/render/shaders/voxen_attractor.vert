#version 450
// Dynamic strange attractor trails based on Samuel's published equation:
// P += cos(P.zxy + T) - S * P
layout(push_constant) uniform PushConstants {mat4 mvp;vec4 parameters;} pc;
layout(location=0) out vec3 color;
const float TAU=6.2831853;
void main(){
    const int segments=18;
    int line=gl_VertexIndex/2;
    int strand=line/segments;
    int step=line%segments+(gl_VertexIndex%2);
    bool rossler=pc.parameters.w>11.5;
    float a=float(strand)*2.399963;
    vec3 p=vec3(0.36*cos(a),0.36*sin(a),0.2*sin(a*1.7));
    float time=pc.parameters.x*pc.parameters.y;
    float s=mix(0.05,0.2,0.5+0.5*sin(time*0.37+float(strand)*0.03));
    // Integration with bounded timestep for predictable Quest frame time.
    for(int i=0;i<20;++i){
        if(i>=step)break;
        if(rossler){
            // Rossler-inspired bounded swirling field, rendered as animated ribbons.
            // Seed per strand prevents all trajectories collapsing together.
            vec3 d=vec3(-p.y-p.z, p.x+0.22*p.y, 0.20+p.z*(p.x-2.7));
            p+=0.035*d;
            p=clamp(p,vec3(-4.0),vec3(4.0));
        }else{
            p+=0.095*(cos(p.zxy+vec3(time*0.55))-s*p);
        }
    }
    vec3 q=rossler ? (p*0.23+vec3(0.0,0.0,0.12*sin(time*0.65+float(strand)*0.3))) : p*0.32;
    color=(0.35+0.35*cos(TAU*(pc.parameters.z+vec3(0.0,0.23,0.48))))
          *(0.5+0.5*float(step)/float(segments));
    gl_Position=pc.mvp*vec4(q,1.0);
}
