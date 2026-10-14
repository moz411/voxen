float collection_hash(float2 seed) {
    seed=sin(seed*float2(12.45,56.43))*64.7+12.34;
    return frac(seed.x*seed.y);
}
// SHADERed EmCYTuRzv2: animated smooth value noise; six octaves.
// Source algorithm preserved, adapted from fullscreen UV to voxel-local coordinates.
float collection_noise(float2 uv) {
    float n=0.0, amp=1.0, den=0.0;
    [unroll] for(int i=0;i<6;i++) {
        float2 f=frac(uv), id=floor(uv);
        float2 w=f*f*(3.0-2.0*f);
        float bl=collection_hash(id), br=collection_hash(id+float2(1,0));
        float tl=collection_hash(id+float2(0,1)), tr=collection_hash(id+float2(1,1));
        n+=lerp(lerp(bl,br,w.x),lerp(tl,tr,w.x),w.y)*amp;
        den+=amp; amp*=0.5; uv*=2.0;
    }
    return n/den;
}
