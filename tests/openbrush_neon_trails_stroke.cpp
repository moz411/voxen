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
    if(m.brushGuid!=NeonTrailsBrushGuid || m.positions.size()!=10*3 ||
       m.indices.size()!=4*6 || m.uv0.size()!=10*4)
        throw std::runtime_error("disconnected strokes joined");
    for(auto idx:m.indices)if(idx>=10)throw std::runtime_error("index");
    // Catmull-Rom preserves endpoints and yields continuous shared joints.
    if(std::abs(m.normals[0]-a.center[0])>1e-5f ||
       std::abs(m.normals[(10-1)*3]-b.center[0])>1e-5f)
        throw std::runtime_error("endpoint drift");
    SmokeStrokePoint bend{};bend.center={0.07f,0.025f,0.f};
    auto curved=makeNeonTrailsStrokeScene({a,b,bend});
    if(curved.meshes.size()!=1 || curved.meshes[0].indices.size()!=8*6)
        throw std::runtime_error("Catmull segment count");
    const auto& curve=curved.meshes[0];
    // One shared row at the segment junction; no separate seam vertices.
    if(std::abs(curve.normals[4*2*3]-b.center[0])>1e-5f ||
       std::abs(curve.normals[4*2*3+1]-b.center[1])>1e-5f)
        throw std::runtime_error("Catmull joint continuity");
    std::cout<<"Neon Trails geometry OK\n";
}
