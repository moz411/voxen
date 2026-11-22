#include <voxen/render/OpenBrushLightningStroke.hpp>
#include <stdexcept>
#include <iostream>
int main() {
    using namespace voxen::render::openbrush;
    SmokeStrokePoint p{};p.size=0.3f;
    auto scene=makeLightningStrokeScene({p});
    if(scene.formatVersion!=2 || scene.meshes.size()!=1)throw std::runtime_error("format");
    const auto& m=scene.meshes[0];
    if(m.brushGuid!=LightningBrushGuid || m.positions.size()!=3*6*2*3 ||
       m.indices.size()!=3*5*6 || m.uv0.size()!=3*6*2*4)
        throw std::runtime_error("lightning geometry");
    for(auto idx:m.indices)if(idx>=m.positions.size()/3)throw std::runtime_error("index");
    if(!makeLightningStrokeScene({}).meshes.empty())throw std::runtime_error("empty");
    std::cout<<"Lightning geometry OK\n";
}
