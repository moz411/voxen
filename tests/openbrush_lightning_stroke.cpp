#include <voxen/render/OpenBrushLightningStroke.hpp>
#include <stdexcept>
#include <iostream>
int main() {
    using namespace voxen::render::openbrush;
    SmokeStrokePoint p{};p.size=0.3f;
    auto scene=makeLightningStrokeScene({p});
    if(scene.formatVersion!=2 || scene.meshes.size()!=1)throw std::runtime_error("format");
    const auto& m=scene.meshes[0];
    // One trunk (9 rows), two branches (6 rows each), four micro-arcs (4 rows each).
    if(m.brushGuid!=LightningBrushGuid || m.positions.size()!=74*3 ||
       m.indices.size()!=180 || m.uv0.size()!=74*4)
        throw std::runtime_error("lightning geometry");
    if(m.normals.size()!=m.positions.size())throw std::runtime_error("centers");
    for(auto idx:m.indices)if(idx>=m.positions.size()/3)throw std::runtime_error("index");
    if(!makeLightningStrokeScene({}).meshes.empty())throw std::runtime_error("empty");
    auto large=makeLightningStrokeScene(std::vector<SmokeStrokePoint>(4096,p));
    if(large.meshes[0].positions.size()/3>1024*74)
        throw std::runtime_error("Lightning LOD budget");
    std::cout<<"Lightning geometry OK\n";
}
