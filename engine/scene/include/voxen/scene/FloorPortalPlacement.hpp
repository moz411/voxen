#pragma once
#include <voxen/scene/EnvironmentSurface.hpp>
#include <voxen/scene/RoomGallery.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <optional>

namespace voxen::gallery {

// Geometric hit on a currently tracked Meta Scene floor rectangle.
// Input/output positions are OpenXR LOCAL meters. A valid hit is NOT a
// persistent anchor; callers must still localize the room before saving it.
struct FloorHit {
    std::uint64_t surfaceId = 0;
    Pose localPose{};
    float distanceMeters = 0;
};

inline std::array<float,3> rotateBy(
    const std::array<float,4>& q, const std::array<float,3>& v) {
    const std::array<float,3> u{q[0],q[1],q[2]};
    const std::array<float,3> t{
        2.f*(u[1]*v[2]-u[2]*v[1]),
        2.f*(u[2]*v[0]-u[0]*v[2]),
        2.f*(u[0]*v[1]-u[1]*v[0])};
    return {v[0]+q[3]*t[0]+u[1]*t[2]-u[2]*t[1],
            v[1]+q[3]*t[1]+u[2]*t[0]-u[0]*t[2],
            v[2]+q[3]*t[2]+u[0]*t[1]-u[1]*t[0]};
}

inline std::optional<FloorHit> intersectTrackedFloor(
    const std::vector<voxen::scene::EnvironmentSurface>& surfaces,
    const std::array<float,3>& origin,
    const std::array<float,3>& direction,
    float maxDistanceMeters = 10.f) {
    if (!std::isfinite(maxDistanceMeters) || maxDistanceMeters<=0) return std::nullopt;
    for (float x:origin) if (!std::isfinite(x)) return std::nullopt;
    for (float x:direction) if (!std::isfinite(x)) return std::nullopt;
    const float len2=direction[0]*direction[0]+direction[1]*direction[1]+direction[2]*direction[2];
    if (len2<1e-8f) return std::nullopt;
    const float invLen=1.f/std::sqrt(len2);
    const std::array<float,3> ray{direction[0]*invLen,direction[1]*invLen,direction[2]*invLen};
    std::optional<FloorHit> nearest;
    for (const auto& s:surfaces) {
        if (!s.tracked || s.classification!=voxen::scene::SurfaceClass::Floor) continue;
        bool finite=true;
        for(float x:s.center) finite &= std::isfinite(x);
        for(float x:s.orientation) finite &= std::isfinite(x);
        for(float x:s.halfExtents) finite &= std::isfinite(x) && x>0;
        if(!finite) continue;
        const auto& q=s.orientation;
        const float norm=q[0]*q[0]+q[1]*q[1]+q[2]*q[2]+q[3]*q[3];
        if(norm<0.99f || norm>1.01f) continue;
        const auto normal=rotateBy(q,{0.f,0.f,1.f});
        if(normal[1]<0.85f) continue; // reject misclassified vertical surfaces
        const float denom=normal[0]*ray[0]+normal[1]*ray[1]+normal[2]*ray[2];
        if(denom>=-1e-5f) continue; // only top-facing surfaces, downward rays
        const float distance=((s.center[0]-origin[0])*normal[0]+
                              (s.center[1]-origin[1])*normal[1]+
                              (s.center[2]-origin[2])*normal[2])/denom;
        if(distance<=0 || distance>maxDistanceMeters ||
           (nearest && distance>=nearest->distanceMeters)) continue;
        const std::array<float,3> point{
            origin[0]+distance*ray[0],origin[1]+distance*ray[1],origin[2]+distance*ray[2]};
        const std::array<float,4> inverse{-q[0],-q[1],-q[2],q[3]};
        const auto relative=rotateBy(inverse,{point[0]-s.center[0],point[1]-s.center[1],point[2]-s.center[2]});
        if(std::abs(relative[0])>s.halfExtents[0] ||
           std::abs(relative[1])>s.halfExtents[1]) continue;
        FloorHit hit;
        hit.surfaceId=s.id;
        hit.distanceMeters=distance;
        hit.localPose.positionMeters=point;
        hit.localPose.orientationXyzw=s.orientation;
        nearest=hit;
    }
    return nearest;
}
} // namespace voxen::gallery
