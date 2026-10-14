// SHADERed jHKMZig75S: layered moving colored particles, localized to each voxel face.
float3 collection_particles(float2 uv, float t) {
    float3 outc=float3(0.1,0.1,0.1);
    float2 p=uv*12.0;
    [loop] for(int j=0;j<10;j++) {
        float fj=j/10.0;
        float r1=(sin(fj*100.0+1.0)+1.0)*0.5;
        float r2=(sin(fj*100.0+7.0)+1.0)*0.5;
        float len=0.5+r2*0.7, maxSize=0.25+r2*0.4, minSize=0.15+r2*0.1;
        float period=lerp(3.0,10.0,r1);
        float phase=frac((t+3.141592+r2*13.0)/period);
        float x=lerp(-6.0-maxSize,6.0+len+minSize,phase);
        [unroll] for(int i=0;i<8;i++) {
            float fi=i/8.0;
            float2 q=float2(x-fi*len,-6.0+12.0*fj+0.6);
            q.y+=0.2*sin(5.0*q.x)+0.6*cos(2.0*q.x)-0.1*cos(-5.0*q.x);
            float radius=lerp(maxSize,minSize,fi);
            float mask=1.0-smoothstep(radius-0.02,radius+0.02,length(p.yx-q));
            float3 tint=float3(lerp(0.1,0.9,r1),lerp(0.4,0.8,r2),fi)*(1.0-0.8*fi);
            outc=lerp(outc,tint,mask);
        }
    }
    return outc;
}

