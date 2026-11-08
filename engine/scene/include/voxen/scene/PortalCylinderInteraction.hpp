#pragma once
#include <voxen/scene/RoomGallery.hpp>
#include <algorithm>
#include <array>
#include <cmath>

namespace voxen::gallery {

// Interaction is the cylinder's floor footprint projected vertically.
// Headset height must not prevent entry: the user walks into the portal,
// rather than ducking below its visible top.
inline bool insidePortalCylinder(const Pose& pose, float scale,
    const std::array<float,3>& head, float radius=0.40f,
    float halfHeight=0.80f) {
    (void)halfHeight; // Visual height only; not an interaction constraint.
    if(!std::isfinite(scale) || scale<=0.f ||
       !std::isfinite(head[0]) || !std::isfinite(head[2]))return false;
    const float dx=head[0]-pose.positionMeters[0];
    const float dz=head[2]-pose.positionMeters[2];
    const float r=radius*std::clamp(scale,0.1f,4.f);
    return dx*dx+dz*dz<=r*r;
}

} // namespace voxen::gallery
