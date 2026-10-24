#pragma once
#include <voxen/scene/FloorPortalPlacement.hpp>
#include <voxen/scene/SceneIds.hpp>
#include <voxen/scene/SceneRepository.hpp>
#include <algorithm>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace voxen::gallery {

// Runtime-only anchor pose in OpenXR LOCAL coordinates, supplied by a verified
// Meta persistent-anchor localization. Never persisted as a LOCAL pose.
struct LocalizedRoom {
    RoomId roomId;
    std::string persistentAnchorId;
    Pose anchorInLocal;
    bool tracked = false;
};

inline Pose compose(const Pose& parent, const Pose& child) {
    Pose result;
    result.orientationXyzw = {
        parent.orientationXyzw[3]*child.orientationXyzw[0] +
        parent.orientationXyzw[0]*child.orientationXyzw[3] +
        parent.orientationXyzw[1]*child.orientationXyzw[2] -
        parent.orientationXyzw[2]*child.orientationXyzw[1],
        parent.orientationXyzw[3]*child.orientationXyzw[1] -
        parent.orientationXyzw[0]*child.orientationXyzw[2] +
        parent.orientationXyzw[1]*child.orientationXyzw[3] +
        parent.orientationXyzw[2]*child.orientationXyzw[0],
        parent.orientationXyzw[3]*child.orientationXyzw[2] +
        parent.orientationXyzw[0]*child.orientationXyzw[1] -
        parent.orientationXyzw[1]*child.orientationXyzw[0] +
        parent.orientationXyzw[2]*child.orientationXyzw[3],
        parent.orientationXyzw[3]*child.orientationXyzw[3] -
        parent.orientationXyzw[0]*child.orientationXyzw[0] -
        parent.orientationXyzw[1]*child.orientationXyzw[1] -
        parent.orientationXyzw[2]*child.orientationXyzw[2]};
    const auto translated=rotateBy(parent.orientationXyzw,child.positionMeters);
    for(int i=0;i<3;++i)
        result.positionMeters[i]=parent.positionMeters[i]+translated[i];
    return result;
}
inline Pose inverse(const Pose& p) {
    Pose result;
    const auto& q=p.orientationXyzw;
    result.orientationXyzw={-q[0],-q[1],-q[2],q[3]};
    result.positionMeters=rotateBy(result.orientationXyzw,
        {-p.positionMeters[0],-p.positionMeters[1],-p.positionMeters[2]});
    return result;
}

struct VisiblePortal {
    InstanceId id;
    SceneId sceneId;
    Pose localPose;
    float scale=1.f;
};

class PortalManager {
public:
    explicit PortalManager(SceneRepository& repository) : repository_(repository) {}

    // Null or untracked localization hides ALL previously saved portals.
    void setLocalization(std::optional<LocalizedRoom> localized) {
        if (!localized || !localized->tracked || localized->roomId.empty() ||
            localized->persistentAnchorId.empty()) {
            localized_.reset();
            gallery_.reset();
            return;
        }
        if(localized_ && gallery_ && localized_->roomId==localized->roomId &&
           localized_->persistentAnchorId==localized->persistentAnchorId) {
            localized_=std::move(localized); // update pose; avoid disk I/O per XR frame
            return;
        }
        auto saved=repository_.loadGallery(localized->roomId);
        if(saved && saved->persistentAnchorId!=localized->persistentAnchorId) {
            localized_.reset();
            gallery_.reset();
            return;
        }
        localized_=std::move(localized);
        gallery_=saved.value_or(RoomGallery{localized_->roomId,
                                            localized_->persistentAnchorId,{}});
    }

    void reloadGallery() {
        if(!localized_)return;
        auto current=*localized_;
        localized_.reset();
        gallery_.reset();
        setLocalization(std::move(current));
    }

    [[nodiscard]] bool localized() const noexcept { return localized_.has_value(); }

    [[nodiscard]] std::vector<VisiblePortal> visible() const {
        std::vector<VisiblePortal> out;
        if (!localized_ || !gallery_) return out;
        for(const auto& p:gallery_->placements) {
            if(!repository_.loadSceneMetadata(p.sceneId)) continue;
            out.push_back({p.id,p.sceneId,
                           compose(localized_->anchorInLocal,p.roomLocalPose),p.scale});
        }
        return out;
    }

    // A floor hit in LOCAL is converted to anchor-local coordinates before
    // persistence. Never write the hit's LOCAL coordinates directly.
    [[nodiscard]] std::optional<InstanceId> place(
        const SceneId& sceneId, const FloorHit& hit, float scale=1.f) {
        if(!localized_ || !gallery_ || !repository_.loadSceneMetadata(sceneId) ||
           !std::isfinite(scale) || scale<=0 || scale>10) return std::nullopt;
        Placement p;
        p.id=newUuid();
        p.sceneId=sceneId;
        p.roomLocalPose=compose(inverse(localized_->anchorInLocal),hit.localPose);
        p.scale=scale;
        gallery_->placements.push_back(p);
        if(!repository_.saveGallery(*gallery_)) {
            gallery_->placements.pop_back();
            return std::nullopt;
        }
        return p.id;
    }

    // Select the closest placed ring to a tracked floor hit (in LOCAL meters).
    [[nodiscard]] std::optional<InstanceId> nearest(
        const FloorHit& hit, float maxDistanceMeters=0.6f) const {
        if(!localized_ || !gallery_)return std::nullopt;
        const auto candidates=visible();
        float best=maxDistanceMeters*maxDistanceMeters;
        std::optional<InstanceId> found;
        for(const auto& p:candidates) {
            const auto& a=p.localPose.positionMeters;
            const auto& b=hit.localPose.positionMeters;
            const float dx=a[0]-b[0],dy=a[1]-b[1],dz=a[2]-b[2];
            const float d2=dx*dx+dy*dy+dz*dz;
            if(d2<best) {best=d2;found=p.id;}
        }
        return found;
    }

    [[nodiscard]] bool move(const InstanceId& id, const FloorHit& hit) {
        if(!localized_ || !gallery_)return false;
        auto it=std::find_if(gallery_->placements.begin(),gallery_->placements.end(),
            [&](const Placement& p){return p.id==id;});
        if(it==gallery_->placements.end())return false;
        const Pose previous=it->roomLocalPose;
        it->roomLocalPose=compose(inverse(localized_->anchorInLocal),hit.localPose);
        if(repository_.saveGallery(*gallery_))return true;
        it->roomLocalPose=previous;
        return false;
    }

    [[nodiscard]] bool erase(const InstanceId& id) {
        if(!localized_ || !gallery_) return false;
        auto it=std::find_if(gallery_->placements.begin(),gallery_->placements.end(),
            [&](const Placement& p){return p.id==id;});
        if(it==gallery_->placements.end()) return false;
        const auto removed=*it;
        const auto offset=static_cast<std::size_t>(it-gallery_->placements.begin());
        gallery_->placements.erase(it);
        if(repository_.saveGallery(*gallery_)) return true;
        gallery_->placements.insert(gallery_->placements.begin()+offset,removed);
        return false;
    }

private:
    SceneRepository& repository_;
    std::optional<LocalizedRoom> localized_;
    std::optional<RoomGallery> gallery_;
};
} // namespace voxen::gallery
