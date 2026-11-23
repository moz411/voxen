#version 450
layout(location=0) in vec4 vColor;
layout(location=1) in vec3 vNormal;
layout(location=0) out vec4 outColor;
void main() {
    vec3 n=normalize(vNormal);
    vec3 light=normalize(vec3(-.45,.85,.32));
    float diffuse=max(dot(n,light),0.0);
    float brightness=.30+.70*diffuse;
    vec3 color=vColor.rgb*brightness;
    // Contrasting rooftops and dark vertical facades give the city depth.
    if(n.y>.5)color*=1.25;
    outColor=vec4(color,1.0);
}
