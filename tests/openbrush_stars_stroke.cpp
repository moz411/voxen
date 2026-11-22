#include <voxen/render/OpenBrushStarsStroke.hpp>
#include <cmath>
#include <stdexcept>
#include <iostream>
int main() {
    using namespace voxen::render::openbrush;
    SmokeStrokePoint p{};
    p.rgb={0.25f,0.5f,0.75f};
    auto scene=makeStarsStrokeScene({p});
    if(scene.meshes.size()!=1)throw std::runtime_error("stars mesh");
    const auto& m=scene.meshes.front();
    if(m.positions.size()!=3*2*8*3 || m.indices.size()!=3*2*6*3)
        throw std::runtime_error("stars geometry");
    if(std::abs(m.colors[0]-0.25f)>1e-5f)throw std::runtime_error("RGB");
    for(auto i:m.indices)if(i>=m.positions.size()/3)throw std::runtime_error("index");
    std::cout<<"Stars geometry OK\n";
}
