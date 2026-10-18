#version 450
// STEP 2 stereo-volume diagnostic: analytic ray/sphere intersection.
// Zero Mandelbulb fractal iterations. If visible, eye-local ray setup works.
layout(location=0) in vec3 localPosition;
layout(location=0) out vec4 outColor;
layout(push_constant) uniform PushConstants {
    mat4 mvp;
    vec4 parameters;
    vec4 eyeLocal;
} pc;
void main(){
    vec3 eye=pc.eyeLocal.xyz;
    vec3 dir=normalize(localPosition-eye);
    // Unit-radius local sphere, concentric with invisible cube proxy.
    float b=dot(eye,dir);
    float c=dot(eye,eye)-0.75*0.75;
    float discriminant=b*b-c;
    if(discriminant<0.0){
        // The cube must ALWAYS be visible, even if this ray misses the sphere.
        outColor=vec4(1.0,0.0,0.8,1.0);
        return;
    }
    float d=sqrt(discriminant);
    float hit=max(0.0,-b-d);
    if(hit<=0.0001)hit=max(0.0,-b+d);
    vec3 point=eye+dir*hit;
    vec3 normal=normalize(point);
    float light=0.3+0.7*max(dot(normal,normalize(vec3(0.3,0.7,1.0))),0.0);
    outColor=vec4(0.15*light,0.9*light,0.55*light,1.0);
}
