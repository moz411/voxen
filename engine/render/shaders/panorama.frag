#version 450
layout(set=0,binding=0) uniform sampler2D panorama;
layout(push_constant) uniform Parameters {
    mat4 inverseViewProjectionRotation;
    vec4 controls; // x yaw radians, y horizontal coverage radians, z vertical coverage radians, w reserved
} pc;
layout(location=0) in vec2 clipXY;
layout(location=0) out vec4 outColor;
const float PI=3.141592653589793;
void main() {
    // Reconstruct direction in tracking space from the per-eye inverse VP rotation.
    vec4 farPoint=pc.inverseViewProjectionRotation*vec4(clipXY,1.0,1.0);
    vec3 direction=normalize(farPoint.xyz/max(abs(farPoint.w),1e-6));
    float longitude=atan(direction.x,-direction.z)-pc.controls.x;
    longitude=mod(longitude+PI,2.0*PI)-PI;
    float latitude=asin(clamp(direction.y,-1.0,1.0));
    float hfov=clamp(pc.controls.y,0.1,2.0*PI);
    float vfov=clamp(pc.controls.z,0.1,PI);
    // For partial smartphone panoramas, do not falsely wrap the left/right edges.
    float u=longitude/hfov+0.5;
    float v=0.5-latitude/vfov;
    vec2 bounded=clamp(vec2(u,v),vec2(0.0),vec2(1.0));
    vec3 rgb=texture(panorama,bounded).rgb;
    // A gentle edge falloff avoids abrupt hard seams at missing angles.
    float outside=max(max(-u,u-1.0),max(-v,v-1.0));
    float fade=1.0-smoothstep(0.0,0.25,outside);
    vec3 horizon=vec3(0.035,0.055,0.075);
    outColor=vec4(mix(horizon,rgb,fade),1.0);
}
