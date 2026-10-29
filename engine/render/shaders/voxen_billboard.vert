#version 450
// Stereo-correct camera-facing procedural quad for image-space Voxen effects.
// Construct vertices in clip space from the projected local basis; the centre
// retains its original per-eye depth and the size follows the model scale.
layout(push_constant) uniform PushConstants { mat4 mvp; vec4 parameters; } pc;
layout(location=0) out vec2 uv;
const vec2 corners[6] = vec2[6](
    vec2(-1.,-1.), vec2(1.,-1.), vec2(1.,1.),
    vec2(-1.,-1.), vec2(1.,1.), vec2(-1.,1.)
);
void main() {
    uv=corners[gl_VertexIndex];
    vec4 center=pc.mvp*vec4(0.,0.,0.,1.);
    vec2 extent=vec2(length(pc.mvp[0].xy),length(pc.mvp[1].xy))*0.5;
    gl_Position=center+vec4(uv*extent,0.,0.);
}
