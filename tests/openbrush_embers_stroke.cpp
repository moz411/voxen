#include <voxen/render/OpenBrushEmbersStroke.hpp>
#include <stdexcept>
#include <iostream>
int main() {
    using namespace voxen::render::openbrush;
    SmokeStrokePoint p{};
    p.rgb={1.f,0.4f,0.05f};
    const auto scene=makeEmbersStrokeScene({p});
    if(scene.meshes.size()!=1)throw std::runtime_error("mesh");
    const auto& m=scene.meshes.front();
    if(m.positions.size()!=7*4*3 || m.indices.size()!=7*6)
        throw std::runtime_error("geometry");
    if(m.colors[0]!=1.f)throw std::runtime_error("RGB");
    for(auto idx:m.indices)if(idx>=m.positions.size()/3)throw std::runtime_error("index");
    std::cout<<"Embers geometry OK\n";
}
