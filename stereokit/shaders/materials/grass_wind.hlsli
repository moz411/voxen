// Godot grass/wind shader port for StereoKit voxels.
// Procedural value noise replaces the three sampler2D inputs not supplied with the source.
// Origin-relative displacement avoids moving an entire world-space artwork.
float grass_hash(float2 p) { return frac(sin(dot(p,float2(127.1,311.7)))*43758.5453); }
float grass_noise(float2 p) {
    float2 i=floor(p), f=frac(p);
    f=f*f*(3.0-2.0*f);
    return lerp(lerp(grass_hash(i),grass_hash(i+float2(1,0)),f.x),
                lerp(grass_hash(i+float2(0,1)),grass_hash(i+float2(1,1)),f.x),f.y);
}
float grass_combined(float3 world) {
    float n1=grass_noise(world.xz/20.0);
    float n2=grass_noise(world.xz/20.0+float2(17.4,5.9));
    return saturate((0.5+n1)*n2);
}
float grass_wind(float3 origin, float t) {
    return grass_noise(origin.xz/20.0+float2(0.03,0.03)*t);
}
float3 grass_displace(float3 local, float3 origin, float t) {
    float combined=grass_combined(origin);
    float wind=grass_wind(origin,t);
    float verticalScale=lerp(0.5,1.5,1.0-combined)+lerp(-0.2,0.2,wind);
    float bias=wind-0.5;
    float shaped=bias<0.0 ? lerp(0.0,-0.2,-bias*2.0) : bias*bias*2.0;
    float speed=max(0.0,0.005*(1.0-shaped*0.5));
    float amount=0.2*(1.0+shaped*3.0);
    // Godot input UV.y denotes blade height, but voxels use local Y instead.
    float baseWeight=1.0-saturate(local.y+0.5);
    local.y *= verticalScale;
    float sway=sin(t*speed)*amount*baseWeight;
    local.x+=sway*0.70710678;
    local.z+=sway*0.70710678;
    return local;
}
float3 grass_color(float3 world,float localHeight,float t) {
    float h=saturate(localHeight+0.5);
    float3 bottom=float3(0.025,0.15,0.035);
    float3 top=float3(0.40,0.70,0.15);
    float3 variation1=float3(0.15,0.46,0.055);
    float3 variation2=float3(0.64,0.72,0.19);
    float3 base=lerp(top,bottom,h);
    float n1=grass_noise(world.xz/20.0);
    float n2=grass_noise(world.xz/20.0+float2(17.4,5.9));
    base=lerp(base,lerp(variation1,bottom,h),n1);
    base=lerp(base,lerp(variation2,bottom,h),n2);
    float wind=grass_wind(world,t);
    float shadow=smoothstep(0.15,0.45,abs(wind-0.5)*0.8)*0.4;
    base=lerp(base,base*float3(0.0,0.2,0.0),shadow);
    return saturate(base*lerp(0.9,1.1,wind));
}
