#include <voxen/render/OpenBrushBubblesStroke.hpp>
#include <stdexcept>
#include <iostream>
int main() {
    using namespace voxen::render::openbrush;
    SmokeStrokePoint p{};
    p.rgb={0.3f,0.8f,1.f};
    auto scene=makeBubblesStrokeScene({p});
    if(scene.meshes.size()!=1)throw std::runtime_error("mesh");
    const auto& m=scene.meshes.front();
    if(m.positions.size()!=3*17*3 || m.indices.size()!=3*16*3)
        throw std::runtime_error("geometry");
    if(m.colors[0]!=0.3f)throw std::runtime_error("RGB");
    for(auto idx:m.indices)if(idx>=m.positions.size()/3)throw std::runtime_error("index");
    std::cout<<"Bubbles geometry OK\n";
}
