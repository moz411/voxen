#include <voxen/scene/SceneRepository.hpp>
#include <cassert>
#include <filesystem>
#include <iostream>

int main() {
    using namespace voxen::gallery;
    auto root = std::filesystem::temp_directory_path() / "voxen_gallery_repository_test";
    std::filesystem::remove_all(root);
    SceneRepository repo(root);
    for (int i=0; i<3; ++i) {
        SceneRecord s;
        s.id="scene-"+std::to_string(i);
        s.displayName="Test "+std::to_string(i);
        s.payloadAsset="payload-rev-1.bin";
        const std::string payload("shader\0audio", 12);
        assert(repo.saveScene(s,payload));
        auto restored=repo.loadScene(s.id);
        assert(restored && restored->second==payload && restored->first.displayName==s.displayName);
    }
    assert(repo.listScenes().size()==3);
    auto generated = SceneRepository::newScene("Generated");
    assert(generated.id.size()==36);
    assert(repo.saveScene(generated,"scene-data"));
    assert(repo.loadScene(generated.id)->second=="scene-data");
    assert(repo.listScenes().size()==4);
    RoomGallery room;
    room.roomId="test-room";
    room.persistentAnchorId="persistent-anchor";
    for (int i=0; i<3; ++i) {
        Placement p;
        p.id="portal-"+std::to_string(i);
        p.sceneId="scene-"+std::to_string(i);
        p.roomLocalPose.positionMeters={float(i),0.f,2.f};
        room.placements.push_back(p);
    }
    assert(repo.saveGallery(room));
    RoomGallery invalid=room;
    invalid.placements[0].sceneId="missing-scene";
    assert(!repo.saveGallery(invalid));
    auto restored=repo.loadGallery(room.roomId);
    assert(restored && restored->placements.size()==3);
    assert(restored->placements[2].roomLocalPose.positionMeters[0]==2.f);
    assert(!repo.loadScene("../invalid"));
    assert(!repo.loadGallery("../invalid"));
    Lifecycle lifecycle;
    assert(lifecycle.requestEnter("scene-0"));
    assert(lifecycle.completeEnter());
    assert(lifecycle.requestLeave());
    assert(lifecycle.completeLeave());
    std::filesystem::remove_all(root);
    std::cout << "SceneRepository: PASS\n";
}
