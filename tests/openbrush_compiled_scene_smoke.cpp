// Standalone host-side VXMSH001 smoke test: compile with C++17, no Vulkan SDK.
// Usage: c++ -std=c++17 -Iengine/render/include tests/openbrush_compiled_scene_smoke.cpp -o /tmp/vxmesh-smoke
//        /tmp/vxmesh-smoke /tmp/scene.vxmesh
#include <voxen/render/OpenBrushCompiledScene.hpp>
#include <cstdint>
#include <iostream>
#include <stdexcept>
int main(int argc,char** argv) {
    if(argc!=2){std::cerr<<"usage: vxmesh-smoke scene.vxmesh\n";return 2;}
    try {
        const auto scene=voxen::render::openbrush::readCompiledScene(argv[1]);
        std::uint64_t vertices=0,indices=0;
        for(const auto& mesh:scene.meshes){
            const auto n=mesh.positions.size()/3;
            if(mesh.positions.size()!=n*3 || mesh.normals.size()!=n*3 ||
               mesh.colors.size()!=n*4 || mesh.uv0.size()!=n*2 ||
               mesh.indices.size()%3!=0)
                throw std::runtime_error("Invalid vertex attributes or triangle count");
            vertices+=n;indices+=mesh.indices.size();
        }
        std::cout<<"meshes="<<scene.meshes.size()<<" vertices="<<vertices
                 <<" triangles="<<indices/3<<"\n";
        return 0;
    } catch(const std::exception& e){
        std::cerr<<"VXMSH validation failed: "<<e.what()<<"\n";
        return 1;
    }
}
