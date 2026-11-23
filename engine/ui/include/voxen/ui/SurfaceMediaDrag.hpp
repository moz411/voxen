#pragma once
#include <voxen/ui/SurfaceMediaEditor.hpp>
#include <voxen/scene/EnvironmentSurface.hpp>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>

namespace voxen::ui {

// Transient interaction; never mutates the persistent assignment map.
// Depth is measured along the controller ray, not the surface normal.
struct SurfaceMediaHit {
    std::string uuid;
    std::uint64_t id=0;
    float depth=0.f;
};

inline std::optional<SurfaceMediaHit> hitMediaSurface(
    const voxen::scene::EnvironmentSurface& surface,
    const std::array<float,3>& origin,const std::array<float,3>& direction) {
    if(!surface.tracked || surface.persistentUuid.empty() ||
       surface.classification==voxen::scene::SurfaceClass::Unknown ||
       surface.halfExtents[0]<=.05f || surface.halfExtents[1]<=.05f)return std::nullopt;
    const auto& q=surface.orientation;
    const std::array<float,3> x{1.f-2.f*(q[1]*q[1]+q[2]*q[2]),
        2.f*(q[0]*q[1]+q[2]*q[3]),2.f*(q[0]*q[2]-q[1]*q[3])};
    const std::array<float,3> y{2.f*(q[0]*q[1]-q[2]*q[3]),
        1.f-2.f*(q[0]*q[0]+q[2]*q[2]),2.f*(q[1]*q[2]+q[0]*q[3])};
    const std::array<float,3> z{2.f*(q[0]*q[2]+q[1]*q[3]),
        2.f*(q[1]*q[2]-q[0]*q[3]),1.f-2.f*(q[0]*q[0]+q[1]*q[1])};
    const auto dot=[](const std::array<float,3>& a,const std::array<float,3>& b){
        return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];};
    const std::array<float,3> delta{surface.center[0]-origin[0],
        surface.center[1]-origin[1],surface.center[2]-origin[2]};
    const float denom=dot(direction,z);
    if(std::fabs(denom)<.0001f)return std::nullopt;
    const float depth=dot(delta,z)/denom;
    if(!std::isfinite(depth) || depth<.15f || depth>8.1f)return std::nullopt;
    const std::array<float,3> hit{origin[0]+depth*direction[0]-surface.center[0],
        origin[1]+depth*direction[1]-surface.center[1],
        origin[2]+depth*direction[2]-surface.center[2]};
    if(std::fabs(dot(hit,x))>surface.halfExtents[0]+.025f ||
       std::fabs(dot(hit,y))>surface.halfExtents[1]+.025f)return std::nullopt;
    return SurfaceMediaHit{surface.persistentUuid,surface.id,depth};
}

struct SurfaceMediaDrag {
    bool active=false;
    std::optional<SurfaceMediaHit> snapped;
    SurfaceMedia media;
    float visibleDistance=.25f;
    // Allow a 5cm retreat to break adhesion without flicker at the plane.
    static constexpr float releaseHysteresis=.05f;

    void begin(SurfaceMedia selected,float distance) {
        active=true;media=std::move(selected);snapped.reset();visibleDistance=distance;
    }
    void cancel() {active=false;snapped.reset();}
    void update(float requestedDistance,const std::optional<SurfaceMediaHit>& hit) {
        if(!active)return;
        if(snapped) {
            if(!hit || hit->uuid!=snapped->uuid ||
               requestedDistance<hit->depth-releaseHysteresis)snapped.reset();
            else snapped=*hit;
        }
        if(!snapped && hit && requestedDistance>=hit->depth)snapped=*hit;
        visibleDistance=snapped?std::min(requestedDistance,snapped->depth):requestedDistance;
    }
    // Exactly one undoable assignment is generated on release, never on update.
    bool finish(SurfaceMediaEditor& editor) {
        if(!active)return false;
        active=false;
        const auto target=snapped;
        snapped.reset();
        return target && editor.assign(target->uuid,media);
    }
};
} // namespace voxen::ui
