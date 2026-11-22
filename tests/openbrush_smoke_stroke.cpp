#include <voxen/render/OpenBrushSmokeStroke.hpp>
#include <cmath>
#include <iostream>
#include <stdexcept>
using namespace voxen::render::openbrush;
int main() {
    SmokeStrokePoint a{{0.f,0.f,0.f},.2f,0.f,1.f};
    SmokeStrokePoint b{{.01f,0.f,0.f},.2f,0.f,1.f};
    if(shouldSampleSmoke(a,b))throw std::runtime_error("oversampling");
    b.center[0]=.03f;
    if(!shouldSampleSmoke(a,b))throw std::runtime_error("missing sample");
    auto scene=makeSmokeStrokeScene({a,b});
    if(scene.formatVersion!=2 || scene.meshes.size()!=1)throw std::runtime_error("format");
    const auto& m=scene.meshes[0];
    if(m.brushGuid!=SmokeBrushGuid || m.positions.size()!=24 ||
       m.normals.size()!=24 || m.colors.size()!=32 ||
       m.uv0.size()!=32 || m.indices.size()!=12)throw std::runtime_error("layout");
    for(auto i:m.indices)if(i>=8)throw std::runtime_error("index");
    for(int i=0;i<4;i++)if(std::abs(m.normals[i*3]-a.center[0])>1e-6)
        throw std::runtime_error("center semantics");
    std::cout<<"OpenBrush Smoke stroke geometry OK\n";
}
