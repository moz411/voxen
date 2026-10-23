#include <voxen/scene/MetaFloorLocalization.hpp>
#include <cassert>
#include <cmath>
#include <iostream>
int main() {
    using namespace voxen;
    scene::EnvironmentSurface floor;
    floor.classification=scene::SurfaceClass::Floor;
    floor.tracked=true;
    floor.persistentUuid="0123456789abcdef0123456789abcdef";
    floor.center={2.f,0.f,3.f};
    floor.orientation={-0.70710678f,0.f,0.f,0.70710678f};
    auto localized=gallery::localizedFloorAnchor({floor});
    assert(localized && localized->tracked);
    assert(localized->roomId=="floor-"+floor.persistentUuid);
    assert(localized->persistentAnchorId==floor.persistentUuid);
    assert(localized->anchorInLocal.positionMeters[0]==2.f);
    floor.tracked=false;
    assert(!gallery::localizedFloorAnchor({floor}));
    floor.tracked=true;
    floor.persistentUuid="not-a-uuid";
    assert(!gallery::localizedFloorAnchor({floor}));
    floor.persistentUuid="0123456789abcdef0123456789abcdef";
    floor.classification=scene::SurfaceClass::Wall;
    assert(!gallery::localizedFloorAnchor({floor}));
    std::cout<<"Meta floor localization: PASS\n";
}
