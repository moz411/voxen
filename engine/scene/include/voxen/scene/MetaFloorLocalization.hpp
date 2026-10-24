#pragma once
#include <voxen/scene/EnvironmentSurface.hpp>
#include <voxen/scene/PortalManager.hpp>
#include <algorithm>
#include <cmath>
#include <optional>
#include <string>
#include <vector>

namespace voxen::gallery {

// Conservative first integration: a tracked, persistent Meta FLOOR spatial
// entity serves as the room's origin. It is NOT the Meta Room entity itself.
// Only stable spatial UUIDs are accepted; LOCAL poses are never persisted.
// Multiple floors: stable UUID ordering avoids switching due to discovery order.
inline std::optional<LocalizedRoom> localizedFloorAnchor(
    const std::vector<voxen::scene::EnvironmentSurface>& surfaces) {
    const voxen::scene::EnvironmentSurface* selected=nullptr;
    for(const auto& s:surfaces) {
        if(!s.tracked || s.classification!=voxen::scene::SurfaceClass::Floor ||
           s.persistentUuid.size()!=32 ||
           !std::all_of(s.persistentUuid.begin(),s.persistentUuid.end(),[](char c){
               return (c>='0'&&c<='9')||(c>='a'&&c<='f');
           })) continue;
        float norm=0.f;
        bool finite=true;
        for(float x:s.center) finite &= std::isfinite(x);
        for(float x:s.orientation) {finite &= std::isfinite(x);norm+=x*x;}
        if(!finite || norm<0.99f || norm>1.01f) continue;
        const auto normal=rotateBy(s.orientation,{0.f,0.f,1.f});
        if(normal[1]<0.85f)continue;
        if(!selected || s.persistentUuid<selected->persistentUuid) selected=&s;
    }
    if(!selected)return std::nullopt;
    LocalizedRoom out;
    // Namespace intentionally says floor, not room. Future Meta Room UUID
    // migration must explicitly remap these gallery IDs.
    out.roomId="floor-"+selected->persistentUuid;
    out.persistentAnchorId=selected->persistentUuid;
    out.anchorInLocal.positionMeters=selected->center;
    out.anchorInLocal.orientationXyzw=selected->orientation;
    out.tracked=true;
    return out;
}
} // namespace voxen::gallery
