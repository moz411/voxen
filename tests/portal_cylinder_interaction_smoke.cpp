#include <voxen/scene/PortalCylinderInteraction.hpp>
#include <voxen/scene/PortalProximityAudio.hpp>
#include <cassert>
#include <cmath>

int main() {
    using namespace voxen::gallery;
    Pose p;
    assert(insidePortalCylinder(p,1.f,{0.f,0.f,0.f}));
    assert(insidePortalCylinder(p,1.f,{0.39f,0.79f,0.f}));
    assert(!insidePortalCylinder(p,1.f,{0.41f,0.f,0.f}));
    assert(!insidePortalCylinder(p,1.f,{0.f,0.81f,0.f}));
    assert(insidePortalCylinder(p,2.f,{0.7f,1.5f,0.f}));
    PortalAudio audio;
    audio.filename="tension.mp3";
    assert(portalProximityGain(audio,3.f)==0.f);
    assert(portalProximityGain(audio,0.f)>0.6f);
    assert(portalProximityGain(audio,1.f)>portalProximityGain(audio,2.f));
    audio.filename.clear();
    assert(portalProximityGain(audio,0.f)==0.f);
}
