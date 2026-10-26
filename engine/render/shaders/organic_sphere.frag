#version 450
layout(location=0) in vec3 vNormal;
layout(location=1) in vec3 vPosition;
layout(location=2) in float vPulse;
layout(location=0) out vec4 outColor;
layout(push_constant) uniform PushConstants {
    mat4 mvp;
    vec4 parameters; // time, speed, hue, amplitude
} pc;
vec3 palette(float t){
    return 0.5+0.5*cos(6.2831853*(t+vec3(0.0,0.33,0.67)));
}
void main(){
    vec3 n=normalize(vNormal);
    vec3 light=normalize(vec3(-0.55,0.78,0.5));
    float diffuse=max(dot(n,light),0.0);
    float rim=pow(1.0-abs(n.z),3.0);
    float phase=pc.parameters.z+0.11*vPulse+0.045*pc.parameters.x*pc.parameters.y;
    int material=clamp(int(floor(pc.parameters.z+0.0001)),0,10);
    vec3 base=palette(fract(phase));
    float bands=0.5+0.5*sin(22.0*vPulse+6.0*vPosition.y-pc.parameters.x*pc.parameters.y*2.0);
    vec3 color=base*(0.22+0.68*diffuse)+vec3(0.05,0.30,0.44)*bands*0.32;
    color+=mix(vec3(0.08,0.55,0.8),vec3(0.65,0.22,0.95),bands)*rim*0.48;
    if(material==1){ // EMBER
        float lava=pow(0.5+0.5*sin(14.0*vPulse+5.0*vPosition.y-2.0*pc.parameters.x),3.0);
        color=mix(vec3(0.12,0.012,0.006),vec3(1.0,0.22,0.018),lava);
        color+=vec3(0.4,0.10,0.0)*rim;
    }else if(material==2){ // ABYSS
        float veins=pow(0.5+0.5*sin(22.0*vPulse+9.0*vPosition.x+pc.parameters.x),7.0);
        color=vec3(0.015,0.025,0.085)*(0.3+diffuse)+
              vec3(0.02,0.7,0.95)*(0.65*veins+0.7*rim);
    }else if(material==3){ // MATRIX
        float columns=step(0.83,fract(vPosition.x*23.0+vPosition.z*7.0));
        float drops=pow(0.5+0.5*sin(vPosition.y*35.0+pc.parameters.x*5.0+
                  floor(vPosition.x*23.0)*2.1),8.0);
        color=vec3(0.005,0.035,0.009)+vec3(0.08,0.95,0.19)*
              (columns*drops+0.18*rim);
    }
    // New procedural materials: no sampled images or texture allocations.
    // Inspired by MIT-licensed procedural noise and CC0 toon techniques;
    // implementations below are original GLSL for the Voxen vertex format.
    float time=pc.parameters.x*pc.parameters.y;
    vec3 p=vPosition;
    float facing=clamp(abs(n.z),0.0,1.0);
    if(material==4){ // BUBBLE: thin-film interference, translucent shell
        float edge=pow(1.0-facing,2.4);
        float film=dot(n,vec3(0.32,0.71,0.62))*6.0+
                   4.0*sin(p.y*3.5+time*0.45);
        vec3 rainbow=0.52+0.48*cos(film+vec3(0.0,2.09,4.18));
        float highlight=pow(max(dot(reflect(-light,n),vec3(0,0,1)),0.0),32.0);
        color=mix(vec3(0.07,0.18,0.26),rainbow,0.72)+
              vec3(0.8,0.95,1.0)*highlight;
        outColor=vec4(clamp(color,0.0,1.0),1.0);
        return;
    }else if(material==5){ // ANIME: hard cel-shading and bright rim
        float shade=floor(diffuse*3.0+0.25)/3.0;
        vec3 ink=vec3(0.055,0.045,0.11);
        vec3 cel=mix(vec3(0.22,0.48,0.85),vec3(1.0,0.48,0.76),
                     0.5+0.5*sin(pc.parameters.z*6.283));
        color=mix(ink,cel,0.36+0.64*shade);
        color+=vec3(0.8,0.95,1.0)*step(0.79,1.0-facing)*0.32;
    }else if(material==6){ // MANGA: ink outlines, halftone and crosshatch
        vec2 uv=p.xy*45.0;
        vec2 cell=fract(uv)-0.5;
        float dotPattern=1.0-smoothstep(0.10,0.32,length(cell));
        float hatch=1.0-smoothstep(0.035,0.10,abs(sin((p.x+p.y)*110.0)));
        float shadow=1.0-step(0.43,diffuse);
        float ink=max(step(0.76,1.0-facing),shadow*mix(dotPattern,hatch,0.42));
        color=mix(vec3(0.95,0.92,0.83),vec3(0.025,0.035,0.07),ink);
    }else if(material==7){ // MARBLE: flowing domain-warped veins
        vec3 q=p*5.0+vec3(0.15*time,-0.1*time,0.07*time);
        float warp=sin(q.y*2.3+sin(q.z*2.8+time))*0.75+
                   cos(q.x*2.1-q.z*1.4)*0.35;
        float vein=pow(0.5+0.5*cos(q.x*4.5+warp*5.0),9.0);
        vec3 stone=mix(vec3(0.045,0.07,0.15),vec3(0.2,0.7,0.78),
                       0.5+0.5*sin(warp+q.z));
        color=stone*(0.25+0.75*diffuse)+vec3(0.95,0.78,0.5)*vein;
    }else if(material==10){ // MAGMA: procedural flowing lava with dark crust
        vec3 q=p*5.5;
        q+=0.28*vec3(sin(q.y*1.7+time*0.38),
                      cos(q.z*1.9-time*0.28),
                      sin(q.x*1.5+time*0.23));
        float noise=0.0,amp=0.5;
        for(int i=0;i<4;++i){
            noise+=amp*(sin(q.x)*sin(q.y+0.71*q.z)*cos(q.z+0.53*q.x));
            q=q*1.93+vec3(1.7,2.3,0.8);amp*=0.5;
        }
        float vein=abs(sin(noise*5.0+vPulse*2.7+time*0.75));
        float heat=1.0-smoothstep(0.12,0.63,vein);
        heat=clamp(heat+0.13*sin(time*1.3+noise*12.0),0.0,1.0);
        vec3 rock=vec3(0.018,0.013,0.014)*(0.40+0.60*diffuse);
        vec3 magma=mix(vec3(0.45,0.025,0.001),vec3(1.8,0.75,0.11),
                       smoothstep(0.18,0.95,heat));
        color=mix(rock,magma,heat);
        color+=vec3(0.65,0.12,0.009)*pow(heat,3.0)*rim;
        // ACES filmic tonemapping + display gamma, no multipass glow cost.
        color=clamp((color*(2.51*color+0.03))/
                    (color*(2.43*color+0.59)+0.14),0.0,1.0);
        color=pow(color,vec3(0.4545));
    }else if(material==9){ // FIRE HANDS: procedural flame field
        float t=pc.parameters.x*1.7;
        vec3 q=vPosition;
        float turbulence=sin(q.x*15.0+t*2.0)*sin(q.z*13.0-t)+
                         0.5*sin(q.y*24.0-q.x*9.0+t*3.0);
        float flame=clamp(0.55+0.35*turbulence+q.y*0.4,0.0,1.0);
        color=mix(vec3(0.9,0.045,0.005),vec3(1.0,0.72,0.10),flame);
        color+=vec3(0.16,0.32,0.8)*pow(1.0-facing,2.0);
        outColor=vec4(clamp(color,0.0,1.0),
                      1.0);
        return;
    }else if(material==8){ // CELLULAR: animated glowing Voronoi-like cells
        vec3 q=p*7.0;
        vec3 cell=floor(q);
        float nearest=10.0;
        // Eight neighboring cells only: mobile GPU budget.
        for(int x=0;x<=1;++x)
        for(int y=0;y<=1;++y)
        for(int z=0;z<=1;++z){
            vec3 c=cell+vec3(x,y,z);
            vec3 seed=0.5+0.35*sin(vec3(
                dot(c,vec3(13.1,17.7,29.3)),
                dot(c,vec3(23.7,11.9,31.1)),
                dot(c,vec3(19.3,37.1,7.7)))+time*0.32);
            nearest=min(nearest,length(q-c-seed));
        }
        float boundary=smoothstep(0.48,0.72,nearest);
        color=mix(vec3(0.015,0.08,0.14),vec3(0.13,0.92,0.85),boundary);
        color*=0.32+0.68*diffuse+0.4*rim;
    }
    outColor=vec4(clamp(color,0.0,1.0),1.0);
}
