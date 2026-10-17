#version 450
layout(location=0) in vec2 uv;
layout(location=0) out vec4 outColor;
layout(set=0,binding=0) uniform sampler2D panelTexture;
void main() {
    vec2 texCoord=vec2(uv.x*0.5+0.5,0.5-uv.y*0.5);
    outColor=texture(panelTexture,texCoord);
    if(outColor.a<0.004) discard;
}
