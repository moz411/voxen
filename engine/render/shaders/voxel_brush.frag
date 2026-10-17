#version 450
layout(location=0) in vec3 tint;
layout(location=1) in vec3 localPoint;
layout(location=2) flat in int materialId;
layout(location=0) out vec4 outColor;
void main(){
    vec3 color=tint;
    if(materialId==1){
        float wave=0.65+0.35*sin(24.0*localPoint.x)*sin(19.0*localPoint.y+8.0*localPoint.z);
        color*=wave;
    }else if(materialId==2){
        float f=0.55+0.45*sin(40.0*length(localPoint)+16.0*atan(localPoint.z,localPoint.x));
        color=mix(tint,vec3(0.8,0.9,1.0),f*0.45);
    }
    outColor=vec4(color,1.0);
}
