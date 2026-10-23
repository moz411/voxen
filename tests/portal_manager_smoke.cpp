#include <voxen/scene/PortalManager.hpp>
#include <cassert>
#include <cmath>
#include <filesystem>
#include <iostream>

int main() {
    using namespace voxen::gallery;
    const auto root=std::filesystem::temp_directory_path()/("voxen_portal_"+newUuid());
    SceneRepository repo(root);
    auto scene=SceneRepository::newScene("Portal demo");
    assert(repo.saveScene(scene,"payload"));
    PortalManager manager(repo);
    FloorHit hit;
    hit.surfaceId=42;
    hit.localPose.positionMeters={3.f,0.f,1.f};
    assert(!manager.place(scene.id,hit));
    assert(manager.visible().empty());

    LocalizedRoom anchor;
    anchor.roomId="room-1";
    anchor.persistentAnchorId="meta-anchor-1";
    anchor.anchorInLocal.positionMeters={2.f,0.f,0.f};
    anchor.tracked=true;
    manager.setLocalization(anchor);
    assert(manager.localized());
    auto portal=manager.place(scene.id,hit);
    assert(portal);
    auto visible=manager.visible();
    assert(visible.size()==1);
    assert(std::abs(visible[0].localPose.positionMeters[0]-3.f)<1e-4f);
    auto persisted=repo.loadGallery(anchor.roomId);
    assert(persisted && persisted->placements.size()==1);
    assert(std::abs(persisted->placements[0].roomLocalPose.positionMeters[0]-1.f)<1e-4f);

    manager.setLocalization(std::nullopt);
    assert(manager.visible().empty());
    assert(!manager.place(scene.id,hit));
    // A new LOCAL origin after headset restart must move the displayed
    // portal according to the freshly localized anchor pose.
    anchor.anchorInLocal.positionMeters={10.f,0.f,0.f};
    manager.setLocalization(anchor);
    visible=manager.visible();
    assert(visible.size()==1);
    assert(std::abs(visible[0].localPose.positionMeters[0]-11.f)<1e-4f);
    assert(manager.erase(*portal));
    assert(manager.visible().empty());
    manager.setLocalization(std::nullopt);
    anchor.persistentAnchorId="wrong-anchor";
    manager.setLocalization(anchor);
    assert(!manager.localized());
    assert(manager.visible().empty());
    std::filesystem::remove_all(root);
    std::cout<<"Portal manager: PASS\n";
}
