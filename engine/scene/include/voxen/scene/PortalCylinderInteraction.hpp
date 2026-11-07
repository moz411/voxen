#pragma once
#include <voxen/scene/RoomGallery.hpp>
#include <algorithm>
#include <array>
#include <cmath>

namespace voxen::gallery {

// Gallery cylinder is centered on its stored pose (Y-up). This is the
// *interaction* volume, independent of its visual tessellation.
inline bool insidePortalCylinder(const Pose& pose, float scale,
    const std::array<float,3>& head, float radius=0.40f,
    float halfHeight=0.80f) {
    if(!std::isfinite(scale) || scale<=0.f)return false;
    const float dx=head[0]-pose.positionMeters[0];
    const float dy=head[1]-pose.positionMeters[1];
    const float dz=head[2]-pose.positionMeters[2];
    const float r=radius*std::clamp(scale,0.1f,4.f);
    const float h=halfHeight*std::clamp(scale,0.1f,4.f);
    return dx*dx+dz*dz<=r*r && std::abs(dy)<=h;
}

} // namespace voxen::gallery
