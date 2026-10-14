float2 rs_rot(float2 p, float a) {
    float s=sin(a), c=cos(a);
    return float2(c*p.x-s*p.y,s*p.x+c*p.y);
}
float2 rs_hash(float2 p) {
    return frac(sin(float2(dot(p,float2(127.1,311.7)),
                           dot(p,float2(269.5,183.3))))*43758.5453);
}
float rs_voronoi(float2 uv) {
    float2 cell=floor(uv*5.0), f=frac(uv*5.0);
    float d=100.0;
    [unroll] for(int y=-1;y<=1;y++) {
        [unroll] for(int x=-1;x<=1;x++) {
            float2 o=float2(x,y), h=rs_hash(cell+o);
            float2 q=o+0.5+0.5*float2(sin(h.x),cos(h.y));
            d=min(d,length(q-f));
        }
    }
    return d;
}
float rs_segment(float2 p,float2 a,float2 b) {
    float2 ab=b-a;
    return length(p-a-ab*saturate(dot(p-a,ab)/max(dot(ab,ab),1e-5)));
}
float3 rs_color(float2 uv,float t) {
    float r=length(uv);
    float3 core=1.0-exp(-float3(0.006,0.01,0.018)/max(abs(r-0.05),0.005));
    float3 edge=1.0-exp(-float3(0.01,0.01,0.05)/max(abs(r-0.30),0.005));
    float2 a=rs_rot(uv*3.0,-t*5.0);
    a=rs_rot(a,5.0*length(a));
    float2 b=rs_rot(uv*3.0,t*5.0);
    b=rs_rot(b,-5.0*length(b));
    float lines=(pow(rs_voronoi(a*1.3),2.5)+pow(rs_voronoi(b*0.5),3.0))
                *(1.0-smoothstep(0.29,0.32,r));
    float3 col=core+edge+lines*float3(0.1,0.4,0.8);
    float2 gl=abs(rs_rot(uv,t*0.3));
    float halo=0.0;
    [unroll] for(int i=1;i<=8;i++) {
        float k=(float)i;
        float offs=sin(t*15.0+frac(k*123.456+867.234)*5.0)*0.01;
        float h=0.32+frac(k*345.67+12.35)*0.1;
        float d=rs_segment(gl,float2(0,0.32+offs),float2(0,h+offs));
        halo+=1.0-exp(-0.0005/max(d,0.001));
        gl=rs_rot(gl,0.349066*frac(k*45.29));
    }
    return saturate(col*(1.0+halo*0.65));
}


