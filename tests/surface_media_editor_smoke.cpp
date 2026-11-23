#include <voxen/ui/SurfaceMediaEditor.hpp>
#include <voxen/ui/CarouselUi.hpp>
#include <voxen/scene/RuntimeSnapshot.hpp>
#include <cassert>
#include <iostream>

int main() {
    using namespace voxen::ui;
    CarouselState carousel;
    carousel.liquidRoom=true;
    carousel.wallShaderMode=19;
    carousel.backgroundSourceKind=0;
    carousel.selectMedia(3,"Shader",12);
    assert(carousel.liquidRoom && carousel.wallShaderMode==19);
    carousel.clearGlobalMedia();
    assert(!carousel.liquidRoom && carousel.backgroundSourceKind==0);
    assert(carousel.applySelectedGlobally());
    assert(carousel.liquidRoom && carousel.wallShaderMode==12);
    carousel.panoramaFiles={"test.jpg"};
    carousel.selectMedia(1,"test.jpg");
    assert(carousel.liquidRoom && carousel.wallShaderMode==12);
    assert(carousel.applySelectedGlobally());
    assert(!carousel.liquidRoom && carousel.backgroundSourceKind==1 && carousel.panoramaSelected==0);
    carousel.clearGlobalMedia();
    assert(carousel.backgroundSourceKind==0 && carousel.panoramaSelected==-1);
    SurfaceMediaEditor e;
    const std::string wall="0123456789abcdef0123456789abcdef";
    const std::string floor="0123456789abcdef0123456789abcdee";
    assert(e.assign(wall,SurfaceMedia{3,"Shader",12}));
    assert(e.assign(floor,SurfaceMedia{1,"floor.jpg",0}));
    assert(e.assign(wall,SurfaceMedia{1,"wall.png",0}));
    assert(e.assign(wall,std::nullopt));
    assert(!e.find(wall) && e.find(floor)->filename=="floor.jpg");
    assert(e.undo() && e.find(wall)->filename=="wall.png");
    assert(e.undo() && e.find(wall)->shaderMode==12);
    assert(e.redo() && e.find(wall)->filename=="wall.png");
    assert(e.assign(wall,SurfaceMedia{2,"movie.mp4",0}));
    assert(!e.canRedo()); // New placement invalidates the redo branch.
    assert(e.find(floor)->filename=="floor.jpg");
    assert(!e.assign("missing",SurfaceMedia{1,"bad/path.jpg",0}));
    voxen::gallery::SavedRuntime snapshot;
    for(const auto& [uuid,media]:e.assignments())
        snapshot.surfaceMedia.push_back({uuid,media.kind,media.filename,media.shaderMode});
    const auto decoded=voxen::gallery::decodeRuntime(voxen::gallery::encodeRuntime(snapshot));
    assert(decoded && decoded->surfaceMedia.size()==2);
    SurfaceMediaEditor::Assignments restored;
    for(const auto& media:decoded->surfaceMedia)
        restored[media.uuid]={media.kind,media.filename,media.shaderMode};
    e.restore(std::move(restored));
    assert(e.find(wall)->filename=="movie.mp4" && e.find(floor)->filename=="floor.jpg");
    assert(!e.canUndo() && !e.canRedo());
    assert(e.assign(wall,std::nullopt) && !e.find(wall));
    assert(e.undo() && e.find(wall)->filename=="movie.mp4");
    assert(e.assign(wall,SurfaceMedia{0,"Sobel",0}));
    assert(e.find(wall)->kind==0 && e.find(floor)->kind==1);
    voxen::gallery::SavedRuntime sobelOverride;
    for(const auto& [uuid,media]:e.assignments())
        sobelOverride.surfaceMedia.push_back({uuid,media.kind,media.filename,media.shaderMode});
    auto sobelReload=voxen::gallery::decodeRuntime(voxen::gallery::encodeRuntime(sobelOverride));
    assert(sobelReload && sobelReload->surfaceMedia.size()==2);
    std::cout<<"Surface media editor: PASS\n";
}
