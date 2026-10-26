#version 450
layout(set=0,binding=0) uniform sampler2D panorama;
layout(push_constant) uniform Parameters {
    vec4 orientation; // OpenXR quaternion (x,y,z,w)
    vec4 tangents; // left,right,down,up
    vec4 controls; // yaw radians, horizontal coverage radians, vertical coverage radians, unused
} pc;
layout(location=0) in vec2 clipXY;
layout(location=0) out vec4 outColor;
const float PI=3.141592653589793;
vec3 rotateQuaternion(vec3 v,vec4 q) {
    return v+2.0*cross(q.xyz,cross(q.xyz,v)+q.w*v);
}
void main() {
    vec2 uv=clipXY*0.5+0.5;
    vec3 cameraRay=normalize(vec3(mix(pc.tangents.x,pc.tangents.y,uv.x),
                                  mix(pc.tangents.z,pc.tangents.w,uv.y),-1.0));
    vec3 direction=rotateQuaternion(cameraRay,normalize(pc.orientation));
    float longitude=atan(direction.x,-direction.z)-pc.controls.x;
    longitude=mod(longitude+PI,2.0*PI)-PI;
    float latitude=asin(clamp(direction.y,-1.0,1.0));
    float hfov=clamp(pc.controls.y,0.1,2.0*PI);
    float vfov=clamp(pc.controls.z,0.1,PI);
    float u=longitude/hfov+0.5;
    float v=0.5-latitude/vfov;
    vec3 rgb=texture(panorama,clamp(vec2(u,v),vec2(0.0),vec2(1.0))).rgb;
    float outside=max(max(-u,u-1.0),max(-v,v-1.0));
    float fade=1.0-smoothstep(0.0,0.2,outside);
    outColor=vec4(mix(vec3(0.035,0.055,0.075),rgb,fade),1.0);
}
