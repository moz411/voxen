#include <voxen/render/OpenBrushChromaticWaveStroke.hpp>
#include <cassert>
#include <iostream>
int main() {
    using namespace voxen::render::openbrush;
    assert(ChromaticWaveStrokePreset==36);
    SmokeStrokePoint a{},b{},c{};
    b.center={.04f,.01f,0.f};c.center={.08f,.02f,0.f};
    assert(makeChromaticWaveStrokeScene({a}).meshes.empty());
    auto scene=makeChromaticWaveStrokeScene({a,b,c});
    assert(scene.formatVersion==2 && scene.meshes.size()==1);
    const auto& mesh=scene.meshes[0];
    assert(mesh.brushGuid==ChromaticWaveBrushGuid);
    assert(mesh.indices.size()==8*6);
    for(auto i:mesh.indices)assert(i<mesh.positions.size()/3);
    std::cout<<"Chromatic Wave geometry OK\n";
}
