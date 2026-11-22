#include <voxen/render/OpenBrushSnowStroke.hpp>
#include <stdexcept>
#include <iostream>
int main() {
    using namespace voxen::render::openbrush;
    SmokeStrokePoint p{};
    p.rgb={0.5f,0.7f,1.f};
    const auto scene=makeSnowStrokeScene({p});
    if(scene.meshes.size()!=1)throw std::runtime_error("mesh");
    const auto& m=scene.meshes.front();
    if(m.positions.size()!=5*2*12*3 || m.indices.size()!=5*2*10*3)
        throw std::runtime_error("geometry");
    if(m.colors[0]!=0.5f)throw std::runtime_error("RGB");
    for(auto idx:m.indices)if(idx>=m.positions.size()/3)throw std::runtime_error("index");
    std::cout<<"Snow geometry OK\n";
}
