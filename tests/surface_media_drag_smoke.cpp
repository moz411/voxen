#include <voxen/ui/SurfaceMediaDrag.hpp>
#include <voxen/ui/CarouselUi.hpp>
#include <cassert>
#include <cmath>
#include <iostream>

int main() {
    using namespace voxen::ui;
    voxen::scene::EnvironmentSurface wall;
    wall.id=42;wall.tracked=true;
    wall.classification=voxen::scene::SurfaceClass::Wall;
    wall.center={0.f,0.f,-2.f};wall.halfExtents={1.f,1.f};
    wall.persistentUuid="0123456789abcdef0123456789abcdef";
    const auto ray=std::array<float,3>{0.f,0.f,-1.f};
    const auto origin=std::array<float,3>{0.f,0.f,0.f};
    const auto hit=hitMediaSurface(wall,origin,ray);
    assert(hit && hit->id==42 && std::fabs(hit->depth-2.f)<.0001f);
    assert(!hitMediaSurface(wall,{2.f,0.f,0.f},ray));
    assert(!hitMediaSurface(wall,origin,{1.f,0.f,0.f}));

    SurfaceMediaEditor editor;
    const SurfaceMedia previous{3,"Shader",7};
    assert(editor.assign(wall.persistentUuid,previous));
    SurfaceMediaDrag drag;
    drag.begin({1,"cloud.jpg",0},.25f);
    drag.update(1.9f,hit);
    assert(!drag.snapped && editor.find(wall.persistentUuid)->shaderMode==7);
    drag.update(2.4f,hit);
    assert(drag.snapped && drag.visibleDistance==2.f);
    drag.update(3.5f,hit);
    assert(drag.snapped && drag.visibleDistance==2.f); // Forward clamp
    drag.update(1.98f,hit);
    assert(drag.snapped); // Hysteresis suppresses frame-to-frame chatter
    drag.update(1.94f,hit);
    assert(!drag.snapped && editor.find(wall.persistentUuid)->shaderMode==7);
    assert(!drag.finish(editor)); // Released off-plane: no history entry
    assert(editor.undo() && !editor.find(wall.persistentUuid));
    assert(editor.redo() && editor.find(wall.persistentUuid)->shaderMode==7);

    drag.begin({1,"cloud.jpg",0},.25f);
    drag.update(2.4f,hit);
    assert(editor.find(wall.persistentUuid)->shaderMode==7); // GPU-only override
    assert(drag.finish(editor));
    assert(!drag.finish(editor)); // One commit per release
    assert(editor.find(wall.persistentUuid)->filename=="cloud.jpg");
    assert(editor.undo() && editor.find(wall.persistentUuid)->shaderMode==7);
    assert(editor.redo() && editor.find(wall.persistentUuid)->filename=="cloud.jpg");

    drag.begin({2,"test.mp4",0},.3f);
    drag.update(2.3f,hit);
    wall.tracked=false;
    drag.update(2.3f,hitMediaSurface(wall,origin,ray));
    assert(!drag.snapped && !drag.finish(editor)); // Tracking lost: cancel
    assert(editor.find(wall.persistentUuid)->filename=="cloud.jpg");
    drag.begin({3,"Shader",11},.3f);
    drag.update(2.3f,hit);
    drag.cancel();
    assert(!drag.finish(editor));
    assert(editor.find(wall.persistentUuid)->filename=="cloud.jpg");
    CarouselState carousel;
    carousel.selectMedia(3,"Shader",12);
    assert(carousel.mediaDragPending);
    carousel.mediaDrag.begin({3,"Shader",12},.25f);
    carousel.mediaDrag.update(2.5f,hit);
    carousel.selectMedia(1,"cloud.jpg");
    assert(carousel.mediaDragPending);
    assert(!carousel.mediaDrag.active && !carousel.mediaDrag.snapped);
    assert(!carousel.liquidRoom); // No global rendering change on select/drag.
    std::cout<<"Surface media drag: PASS\n";
}
