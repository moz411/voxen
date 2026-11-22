#include <voxen/render/OpenBrushMagicDustStroke.hpp>
#include <stdexcept>
#include <iostream>
int main() {
    using namespace voxen::render::openbrush;
    SmokeStrokePoint p{};p.rgb={0.2f,0.7f,1.f};
    auto scene=makeMagicDustStrokeScene({p});
    if(scene.formatVersion!=2 || scene.meshes.size()!=1)throw std::runtime_error("format");
    const auto& m=scene.meshes[0];
    if(m.brushGuid!=MagicDustBrushGuid || m.positions.size()!=7*4*3 ||
       m.indices.size()!=7*6 || m.uv0.size()!=7*4*4)
        throw std::runtime_error("dust geometry");
    for(auto idx:m.indices)if(idx>=28)throw std::runtime_error("index");
    if(m.colors[0]!=0.2f)throw std::runtime_error("color");
    std::cout<<"Magic Dust geometry OK\n";
}
