// Panoramic port of Godot fireworks, based on BigWings / Shadertoy lscGRl.
// The original screen coordinates are replaced with equirectangular sky coordinates.
float3 fireworks_hash31(float p) {
    float3 v=frac(p*float3(0.1031,0.11369,0.13787));
    v+=dot(v,v.yzx+19.19);
    return frac(float3((v.x+v.y)*v.z,(v.x+v.z)*v.y,(v.y+v.z)*v.x));
}
float3 fireworks_explosion(float2 uv,float2 center,float seed,float t) {
    float3 en=fireworks_hash31(seed);
    float3 col=0.0;
    [loop] for(int i=0;i<70;i++) {
        float3 n=fireworks_hash31((float)i+seed*0.013)-0.5;
        float2 start=center-float2(0,t*t*0.1);
        float2 delta=n.xy/max(length(n.xy),0.00001)*n.z;
        float2 end=start+delta;
        float pt=1.0-(t-1.0)*(t-1.0);
        float2 pos=lerp(center,end,pt);
        float size=lerp(0.01,0.005,smoothstep(0.0,0.1,pt));
        size*=1.0-smoothstep(0.1,1.0,pt);
        float sparkle=sin((pt+n.z)*100.0)*0.5+0.5;
        sparkle=pow(max(sparkle,0.00001),pow(en.x,3.0)*50.0)*0.01;
        float appear=smoothstep(en.x-en.z,en.x+en.z,t);
        float disappear=1.0-smoothstep(en.y-en.z,en.y+en.z,t);
        size+=sparkle*appear*disappear;
        float2 d=uv-pos;
        // Keep a bounded bloom radius to avoid blown-out entire sky.
        col+=en*(size*size/max(dot(d,d),0.00003));
    }
    return col;
}
float3 environment_fireworks(float2 uv,float t) {
    // Center fireworks on upper hemisphere; azimuth wraps at the seam.
    float2 p=float2((uv.x-0.5)*1.6,(uv.y-0.5));
    float3 c=0.0;
    [loop] for(int i=0;i<8;i++) {
        float et=t*0.5+(float)i*1234.45235;
        float id=floor(et);
        et=frac(et);
        float2 center=fireworks_hash31(id).xy;
        center.x=(center.x-0.5)*1.6;
        center.y=0.02+center.y*0.42;
        c+=fireworks_explosion(p,center,id,et);
        c+=fireworks_explosion(p+float2(1.6,0),center,id,et);
        c+=fireworks_explosion(p-float2(1.6,0),center,id,et);
    }
    return saturate(c);
}
