// Shared native-brush animation; defaults preserve every existing brush.
vec3 brushMotion(vec3 p, vec4 settings, float phase) {
    int mode=int(settings.y+0.5);
    if(mode==0)return p;
    float t=settings.x*settings.z;
    float radius=0.09;
    if(mode==1)return p+radius*vec3(sin(t*1.7+phase),cos(t*1.3+phase)*0.65,
                                      sin(t*1.1+phase*1.3));
    if(mode==2) {
        float a=t*0.85;
        return p+radius*vec3(cos(a+phase)-cos(phase),0.35*sin(a+phase),
                             sin(a+phase)-sin(phase));
    }
    return p+radius*vec3(sin(t*0.8+phase)*1.5,sin(t*1.2+phase)*0.8,
                         cos(t*0.6+phase)*1.5);
}
vec3 brushCycle(vec3 color,vec4 settings) {
    if(settings.w<=0.0)return color;
    float angle=settings.x*settings.w*6.28318530718;
    float c=cos(angle),s=sin(angle);
    vec3 axis=vec3(0.57735026919);
    return clamp(color*c+cross(axis,color)*s+axis*dot(axis,color)*(1.0-c),0.0,1.0);
}
