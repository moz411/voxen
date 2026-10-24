#include <voxen/scene/FloorPortalPlacement.hpp>
#include <cassert>
#include <cmath>
#include <iostream>
int main() {
    using namespace voxen;
    scene::EnvironmentSurface floor;
    floor.id=42;
    floor.classification=scene::SurfaceClass::Floor;
    floor.tracked=true;
    floor.center={0.f,0.f,0.f};
    floor.halfExtents={2.f,2.f};
    // Local +Z points world +Y: -90 degrees about X.
    floor.orientation={-0.70710678f,0.f,0.f,0.70710678f};
    const std::vector<scene::EnvironmentSurface> surfaces{floor};
    auto hit=gallery::intersectTrackedFloor(surfaces,{0.f,1.5f,0.f},{0.f,-1.f,0.f});
    assert(hit && hit->surfaceId==42);
    assert(std::abs(hit->distanceMeters-1.5f)<1e-4f);
    assert(std::abs(hit->localPose.positionMeters[1])<1e-4f);
    assert(!gallery::intersectTrackedFloor(surfaces,{3.f,1.5f,0.f},{0.f,-1.f,0.f}));
    assert(!gallery::intersectTrackedFloor(surfaces,{0.f,1.5f,0.f},{0.f,1.f,0.f}));
    floor.tracked=false;
    assert(!gallery::intersectTrackedFloor({floor},{0.f,1.f,0.f},{0.f,-1.f,0.f}));
    floor.tracked=true;
    floor.classification=scene::SurfaceClass::Wall;
    assert(!gallery::intersectTrackedFloor({floor},{0.f,1.f,0.f},{0.f,-1.f,0.f}));
    std::cout<<"Floor portal placement: PASS\n";
}
