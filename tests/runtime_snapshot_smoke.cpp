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
    r.cycleWallShaders=true;
    r.backgroundSourceKind=2;
    r.videoFile="ambient.mp4";
    r.panoramaFile="sky.jpg";
    r.differentShaderPerSurface=true;
    r.particleMotionMode=2;
    r.particleMotionSpeed=1.75f;
    r.particleColorCycle=0.4f;
    r.surfaceMedia={{"00112233-4455-6677-8899-aabbccddeeff",1,"sky.jpg",0},
                    {"11112233-4455-6677-8899-aabbccddeeff",3,"Shader",12},
                    {"22112233-4455-6677-8899-aabbccddeeff",2,"ambient.mp4",0}};
    SavedFreeMedia free;
    free.kind=1; free.filename="sky.jpg";
    free.center={1.f,1.4f,-2.f};
    free.orientation={0.f,.7071068f,0.f,.7071068f};
    free.halfExtents={.4f,.25f};
    free.motionMode=2; free.motionSpeed=1.5f;
    r.freeMedia.push_back(free);
    auto bytes=encodeRuntime(r);
    auto loaded=decodeRuntime(bytes);
    assert(loaded && loaded->voxens.size()==1);
    assert(loaded->backgroundSourceKind==2 && loaded->videoFile=="ambient.mp4");
    assert(loaded->panoramaFile=="sky.jpg");
    assert(loaded->cycleWallShaders && loaded->differentShaderPerSurface);
    assert(loaded->surfaceMedia.size()==3);
    assert(loaded->freeMedia.size()==1);
    assert(loaded->freeMedia[0].center==free.center);
    assert(loaded->freeMedia[0].orientation==free.orientation);
    assert(loaded->freeMedia[0].halfExtents==free.halfExtents);
    assert(loaded->freeMedia[0].motionMode==2);
    assert(loaded->surfaceMedia[0].filename=="sky.jpg");
    assert(loaded->surfaceMedia[1].shaderMode==12);
    assert(loaded->surfaceMedia[2].kind==2);
    assert(loaded->particleMotionMode==2 && loaded->particleMotionSpeed==1.75f &&
           loaded->particleColorCycle==0.4f);
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
    // V5 payloads default to uniform per-surface behavior.
    const std::string v5 = "VOXEN_RUNTIME 5\n0\n0\n- 0 220 75\n0 0 0 0.45 0 0 0 0 0.82 1.5 0.8 0\n0 0.42 0 1 30\n";
    auto old=decodeRuntime(v5);
    assert(old && old->cycleWallShaders && !old->differentShaderPerSurface);
    assert(old->backgroundSourceKind==0 && old->videoFile.empty());
    assert(old->particleMotionMode==0 && old->particleColorCycle==0.f);
    r.particleMotionMode=4;
    assert(decodeRuntime(encodeRuntime(r)));
    r.particleMotionMode=5;
    assert(encodeRuntime(r).empty());
    r.particleMotionMode=2;
    // Runtime v9 captures animation and gradient for each individual stroke.
    SavedRuntime strokes;
    SavedVoxen first,second;
    first.preset=27;first.strokeId=41;first.brushMotion=4;
    first.brushMotionSpeed=2.5f;first.brushColorMode=4;
    first.brushEndRgb={.9f,.1f,.3f};
    second.preset=27;second.strokeId=42;second.brushMotion=1;
    second.brushColorMode=2;second.brushColorCycle=1.f;
    strokes.voxens={first,second};
    auto strokeBytes=encodeRuntime(strokes);
    assert(strokeBytes.find("VOXEN_RUNTIME 11")!=std::string::npos);
    auto strokeRoundtrip=decodeRuntime(strokeBytes);
    assert(strokeRoundtrip && strokeRoundtrip->voxens.size()==2);
    assert(strokeRoundtrip->voxens[0].strokeId==41);
    assert(strokeRoundtrip->voxens[0].brushMotion==4);
    assert(strokeRoundtrip->voxens[0].brushMotionSpeed==2.5f);
    assert(strokeRoundtrip->voxens[0].brushColorMode==4);
    assert(strokeRoundtrip->voxens[0].brushEndRgb==first.brushEndRgb);
    assert(strokeRoundtrip->voxens[1].strokeId==42);
    assert(strokeRoundtrip->voxens[1].brushMotion==1);
    assert(strokeRoundtrip->voxens[1].brushColorCycle==1.f);
    // Existing v8 scenes must decode without per-stroke fields.
    const std::string legacyV8="VOXEN_RUNTIME 8\n1\n0\n- 0 220 75\n"
        "0 0 0 0.45 0 0 0 0 0.82 1.5 0.8 0\n"
        "0 0.42 0 0 30 0\n0 -\n2 1.5 0.5\n"
        "27 0 0.18 0.1 0.2 0.3 0 0 0 0 0 0\n";
    auto legacy=decodeRuntime(legacyV8);
    assert(legacy && legacy->voxens.size()==1);
    assert(legacy->voxens[0].brushMotion==2);
    assert(legacy->voxens[0].brushColorCycle==0.5f);
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
