#include <voxen/scene/RuntimeSnapshot.hpp>
#include <cassert>
#include <iostream>
int main() {
    using namespace voxen::gallery;
    SavedRuntime r;
    SavedVoxen v;
    v.preset=4; v.position={1.f,2.f,3.f}; v.velocity={.1f,.2f,.3f};
    v.material=2;
    r.voxens.push_back(v);
    r.musicSource=1; r.musicVolume=.6f; r.motionEnabled=true;
    auto bytes=encodeRuntime(r);
    auto loaded=decodeRuntime(bytes);
    assert(loaded && loaded->voxens.size()==1);
    assert(loaded->voxens[0].position==v.position);
    assert(loaded->voxens[0].velocity==v.velocity);
    assert(loaded->voxens[0].material==2);
    assert(loaded->musicSource==1 && loaded->motionEnabled);
    SavedRuntime cannon;
    cannon.sceneKind=1;
    auto cannonBytes=encodeRuntime(cannon);
    auto cannonLoaded=decodeRuntime(cannonBytes);
    assert(cannonLoaded && cannonLoaded->sceneKind==1 && cannonLoaded->voxens.empty());
    // Old saved scenes remain compatible after introducing runtime v3.
    assert(decodeRuntime("VOXEN_RUNTIME 2\n0\n0 0 0 0.45 0 0 0 0 0.82\n0 0.42 0 0 30\n"));
    // New procedural and OpenBrush presets must survive a scene round-trip.
    SavedRuntime extended;
    extended.voxens.resize(300);
    extended.voxens.back().preset=42;
    extended.voxens.back().material=11;
    auto extendedBytes=encodeRuntime(extended);
    auto extendedLoaded=decodeRuntime(extendedBytes);
    assert(extendedLoaded && extendedLoaded->voxens.size()==300);
    assert(extendedLoaded->voxens.back().preset==42);
    assert(extendedLoaded->voxens.back().material==11);
    // An invalid live object must not prevent other objects from being saved.
    extended.voxens.back().scale=-1.f;
    extended.voxens.back().position[0]=std::numeric_limits<float>::quiet_NaN();
    extended.wallShaderMode=12;
    assert(sanitizeForSave(extended));
    assert(decodeRuntime(encodeRuntime(extended)));
    assert(extended.voxens.back().scale>0.f);
    assert(extended.voxens.back().position[0]==0.f);
    extended.voxens.resize(4097);
    assert(!sanitizeForSave(extended));
    cannon.sceneKind=2;
    assert(encodeRuntime(cannon).empty());
    assert(!decodeRuntime("VOXEN_RUNTIME 1\n999999\n"));
    assert(!decodeRuntime(bytes+"junk"));
    r.voxens[0].scale=-1.f;
    assert(encodeRuntime(r).empty());
    std::cout<<"Runtime snapshot: PASS\n";
}
