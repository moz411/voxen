#include <voxen/render/OpenBrushElectricityStroke.hpp>
#include <cassert>
#include <iostream>
int main() {
    using namespace voxen::render::openbrush;
    assert(ElectricityStrokePreset==37);
    assert(makeElectricityStrokeScene({}).meshes.empty());
    SmokeStrokePoint p{};p.size=6.f;
    auto scene=makeElectricityStrokeScene({p});
    assert(scene.formatVersion==2 && scene.meshes.size()==1);
    const auto& mesh=scene.meshes[0];
    assert(mesh.brushGuid==ElectricityBrushGuid);
    assert(mesh.indices.size()==180);
    for(auto i:mesh.indices)assert(i<mesh.positions.size()/3);
    std::cout<<"Electricity geometry OK\n";
}
