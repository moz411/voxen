#include <voxen/render/OpenBrushNeonTrailsStroke.hpp>
#include <stdexcept>
#include <iostream>
int main() {
    using namespace voxen::render::openbrush;
    SmokeStrokePoint a{},b{},c{};
    b.center={0.04f,0.f,0.f};c.center={2.f,0.f,0.f};
    if(!makeNeonTrailsStrokeScene({a}).meshes.empty())throw std::runtime_error("one point");
    auto scene=makeNeonTrailsStrokeScene({a,b,c});
    if(scene.formatVersion!=2 || scene.meshes.size()!=1)throw std::runtime_error("format");
    const auto& m=scene.meshes[0];
    if(m.brushGuid!=NeonTrailsBrushGuid || m.positions.size()!=12 ||
       m.indices.size()!=6 || m.uv0.size()!=16)
        throw std::runtime_error("disconnected strokes joined");
    for(auto idx:m.indices)if(idx>=4)throw std::runtime_error("index");
    std::cout<<"Neon Trails geometry OK\n";
}
