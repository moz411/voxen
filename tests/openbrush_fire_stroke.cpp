#include <voxen/render/OpenBrushFireStroke.hpp>
#include <cmath>
#include <stdexcept>
#include <iostream>
int main() {
    using namespace voxen::render::openbrush;
    SmokeStrokePoint p{};
    p.rgb={1.f,0.2f,0.03f};
    auto scene=makeFireStrokeScene({p});
    if(scene.meshes.size()!=1)throw std::runtime_error("fire mesh");
    const auto& m=scene.meshes.front();
    // Six tongues, three planes, ten rows of paired vertices, plus
    // five ember billboards (four vertices each).
    if(m.positions.size()!=(6*3*10*2+5*4)*3 ||
       m.indices.size()!=6*3*9*6+5*6)
        throw std::runtime_error("fire geometry");
    if(std::abs(m.colors[0]-1.f)>1e-5f)throw std::runtime_error("RGB");
    for(auto i:m.indices)if(i>=m.positions.size()/3)throw std::runtime_error("index");
    std::cout<<"Fire geometry OK\n";
}
