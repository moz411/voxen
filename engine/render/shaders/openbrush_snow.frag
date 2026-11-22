#version 450
// Analytic six-arm crystal plus soft bloom; no atlas fetches.
layout(location=0) in vec4 vColor;
layout(location=1) in vec2 vDisk;
layout(location=0) out vec4 outColor;
void main() {
    float r=length(vDisk);
    if(r>1.35) discard;
    float angle=atan(vDisk.y,vDisk.x);
    float arms=pow(abs(cos(angle*3.0)),18.0);
    float branches=pow(abs(cos(angle*6.0+0.36)),28.0);
    float body=exp(-28.0*r*r);
    float crystal=(arms*(1.0-smoothstep(0.38,0.98,r))+
                   branches*0.24*(1.0-smoothstep(0.25,0.85,r)));
    float halo=0.18*exp(-5.5*r*r);
    float alpha=clamp((body+crystal*0.72+halo)*vColor.a,0.0,0.92);
    outColor=vec4(mix(vColor.rgb,vec3(0.89,0.96,1.0),0.62+body*0.3),alpha);
}
