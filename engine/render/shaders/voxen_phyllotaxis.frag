#version 450
// Phyllotaxis Spiral: port of user-supplied 100-sphere analytical ray intersection.
layout(location=0) in vec2 uv;
layout(location=0) out vec4 outColor;
layout(push_constant) uniform PushConstants {mat4 mvp;vec4 parameters;} pc;
const float PI=3.14159265;
const int COUNT=100;
void main(){
  float time=pc.parameters.x*pc.parameters.y;
  float ratio=0.2+0.01*time;
  // Scale original radius (up to 8 units) into the small Voxen preview quad.
  float camDistance=12.0;
  float a=time*0.2;
  vec3 ro=vec3(sin(a)*camDistance,2.0,cos(a)*camDistance);
  vec3 forward=normalize(-ro);
  vec3 right=normalize(cross(forward,vec3(0,1,0)));
  vec3 up=normalize(cross(right,forward));
  vec3 rd=normalize(uv.x*right*1.6+uv.y*up*1.6+1.5*forward);
  float best=1e5;
  for(int i=0;i<COUNT;i++){
    float f=float(i);
    float radius=f*.08;
    float angle=f*ratio*2.0*PI;
    vec3 center=vec3(radius*cos(angle),radius*sin(angle),(f-50.0)*.06);
    vec3 oc=ro-center;
    float b=dot(oc,rd);
    float c=dot(oc,oc)-.08*.08;
    float h=b*b-c;
    if(h>0.0){
      float t=-b-sqrt(h);
      if(t>0.0 && t<best)best=t;
    }
  }
  if(best>=1e4)discard;
  outColor=vec4(vec3(1.0),0.96);
}
