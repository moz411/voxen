#pragma once
#include <openxr/openxr.h>
#include <algorithm>
#include <cmath>

namespace voxen::ui {
struct PanelHit {
    bool valid = false;
    float u = 0;
    float v = 0;
    float distance = 0;
};

// XR poses are specified in the same local reference space.
// Panel local coordinates: XY plane, origin center, local +Z toward the user.
// ImGui's (0,0) is top-left; output normalized to [0,1] for DisplaySize mapping.
inline PanelHit rayToPanel(const XrPosef& controller, const XrPosef& panel,
                           float widthMeters, float heightMeters) noexcept {
    if(widthMeters<=0 || heightMeters<=0) return {};
    const auto& cq=controller.orientation;
    const float rx=-2.f*(cq.x*cq.z+cq.w*cq.y);
    const float ry=-2.f*(cq.y*cq.z-cq.w*cq.x);
    const float rz=-(1.f-2.f*(cq.x*cq.x+cq.y*cq.y));
    const auto& q=panel.orientation;
    const float x=-q.x,y=-q.y,z=-q.z,w=q.w;
    const auto rotate=[&](float ax,float ay,float az) {
        const float tx=2.f*(y*az-z*ay),ty=2.f*(z*ax-x*az),tz=2.f*(x*ay-y*ax);
        return XrVector3f{ax+w*tx+y*tz-z*ty,ay+w*ty+z*tx-x*tz,az+w*tz+x*ty-y*tx};
    };
    const auto o=rotate(controller.position.x-panel.position.x,
                        controller.position.y-panel.position.y,
                        controller.position.z-panel.position.z);
    const auto d=rotate(rx,ry,rz);
    if (std::fabs(d.z)<1e-5f) return {};
    const float t=-o.z/d.z;
    if(t<=0.03f || t>5.f) return {};
    const float u=0.5f+(o.x+t*d.x)/widthMeters;
    const float v=0.5f-(o.y+t*d.y)/heightMeters;
    if(u<0 || u>1 || v<0 || v>1) return {};
    return {true,u,v,t};
}
} // namespace voxen::ui
