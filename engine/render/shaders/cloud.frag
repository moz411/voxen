#version 450
layout(location=0) in vec3 vNormal;
layout(location=1) in vec3 vPosition;
layout(location=2) in float vPulse;
layout(location=0) out vec4 outColor;
layout(push_constant) uniform PushConstants {mat4 mvp;vec4 parameters;} pc;
void main(){
    vec3 n=normalize(vNormal);
    vec3 sun=normalize(vec3(0.5,0.75,0.45));
    float light=max(dot(n,sun),0.0);
    float rim=pow(1.0-abs(n.z),2.2);
    float soft=clamp(0.65+0.35*sin(vPulse*14.0+vPosition.x*4.0+
                 vPosition.z*3.0+pc.parameters.x*0.18),0.0,1.0);
    vec3 sky=vec3(0.58,0.66,0.80);
    vec3 sunlight=vec3(1.0,0.96,0.87);
    vec3 color=mix(sky,sunlight,clamp(0.30+0.65*light+0.12*soft,0.0,1.0));
    color+=vec3(0.14,0.18,0.23)*rim*0.30;
    // Alpha blend softens the shell, rather than 128 volumetric ray steps.
    float alpha=clamp(0.64+0.22*soft+0.08*light-0.16*rim,0.40,0.91);
    outColor=vec4(clamp(color,0.0,1.0),alpha);
}
