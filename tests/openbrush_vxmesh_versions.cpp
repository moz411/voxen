// Dependency-free host regression test for VXMSH001 and VXMSH002.
// c++ -std=c++17 -Iengine/render/include tests/openbrush_vxmesh_versions.cpp -o /tmp/vxmesh-versions
// /tmp/vxmesh-versions
#include <voxen/render/OpenBrushCompiledScene.hpp>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using voxen::render::openbrush::readCompiledScene;
static void u32(std::ofstream& o,std::uint32_t x) {
    const char b[4]={char(x),char(x>>8),char(x>>16),char(x>>24)};
    o.write(b,4);
}
static void f32(std::ofstream& o,float x) {
    std::uint32_t bits=0;std::memcpy(&bits,&x,4);u32(o,bits);
}
static void writeFixture(const std::filesystem::path& path,bool v2) {
    std::ofstream o(path,std::ios::binary);
    if(!o)throw std::runtime_error("cannot write fixture");
    o.write(v2?"VXMSH002":"VXMSH001",8);
    u32(o,v2?2:1);u32(o,1);
    std::string guid="70d79cca-b159-4f35-990c-f02193947fe8";
    o.write(guid.data(),36);
    u32(o,3);u32(o,3);
    for(float x:{0.f,0.f,0.f,1.f,1.f,0.f})f32(o,x);
    for(float x:{0.f,0.f,0.f,1.f,0.f,0.f,0.f,1.f,0.f})f32(o,x);
    for(int i=0;i<9;i++)f32(o,0.f);
    for(int i=0;i<12;i++)f32(o,1.f);
    for(int i=0;i<3;i++){
        f32(o,0.25f);f32(o,0.75f);
        if(v2){f32(o,1.25f);f32(o,2.5f);}
    }
    u32(o,0);u32(o,1);u32(o,2);
    if(!o)throw std::runtime_error("fixture write failed");
}
static void verify(const std::filesystem::path& file,bool v2) {
    auto scene=readCompiledScene(file.string());
    if(scene.meshes.size()!=1)throw std::runtime_error("mesh count mismatch");
    const auto& uv=scene.meshes[0].uv0;
    if(uv.size()!=12)throw std::runtime_error("UV length mismatch");
    for(std::size_t i=0;i<3;i++) {
        const float expected[4]={.25f,.75f,v2?1.25f:0.f,v2?2.5f:0.f};
        for(int j=0;j<4;j++)
            if(std::fabs(uv[i*4+j]-expected[j])>1.e-6f)
                throw std::runtime_error("UV round-trip mismatch");
    }
}
int main() {
    const auto base=std::filesystem::temp_directory_path()/"voxen_vxmesh_version_fixture";
    const auto v1=base.string()+".v1";
    const auto v2=base.string()+".v2";
    try{
        writeFixture(v1,false);writeFixture(v2,true);
        verify(v1,false);verify(v2,true);
        std::filesystem::remove(v1);std::filesystem::remove(v2);
        std::cout<<"VXMSH001/VXMSH002 UV compatibility OK\n";
        return 0;
    }catch(const std::exception& e){
        std::filesystem::remove(v1);std::filesystem::remove(v2);
        std::cerr<<e.what()<<"\n";return 1;
    }
}
