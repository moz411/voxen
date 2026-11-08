#pragma once
// VXMSH001/v1 and VXMSH002/v2 offline Open Brush geometry reader.
// No Vulkan dependency. Untrusted input is bounds-checked before allocation.
#include <array>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace voxen::render::openbrush {
struct CompiledMesh {
    std::string brushGuid;
    std::array<float,6> bounds{};
    // UV0 is always 4 floats/vertex in memory; v1 UV.xy is zero-padded.\n    std::vector<float> positions, normals, colors, uv0;
    std::vector<std::uint32_t> indices;
};
struct CompiledScene {
    std::vector<CompiledMesh> meshes;
};
inline CompiledScene readCompiledScene(const std::string& path) {
    std::ifstream file(path,std::ios::binary|std::ios::ate);
    if(!file)throw std::runtime_error("VXMSH: cannot open "+path);
    const auto length=file.tellg();
    if(length<16||length>static_cast<std::streamoff>(512ull*1024*1024))
        throw std::runtime_error("VXMSH: invalid file size");
    file.seekg(0);
    std::vector<std::uint8_t> data(static_cast<std::size_t>(length));
    if(!file.read(reinterpret_cast<char*>(data.data()),static_cast<std::streamsize>(data.size())))
        throw std::runtime_error("VXMSH: short read");
    std::size_t off=0;
    auto take=[&](std::size_t bytes)->const std::uint8_t*{
        if(bytes>data.size()-off)throw std::runtime_error("VXMSH: truncated data");
        auto* p=data.data()+off;off+=bytes;return p;
    };
    const auto* magic=take(8);
    const bool v1=std::memcmp(magic,"VXMSH001",8)==0;
    const bool v2=std::memcmp(magic,"VXMSH002",8)==0;
    if(!v1&&!v2)throw std::runtime_error("VXMSH: bad magic");
    auto readU32=[&](){
        auto* p=take(4);
        return std::uint32_t(p[0])|(std::uint32_t(p[1])<<8)|
               (std::uint32_t(p[2])<<16)|(std::uint32_t(p[3])<<24);
    };
    if(readU32()!=(v2?2u:1u))
        throw std::runtime_error("VXMSH: unsupported version");
    const auto meshCount=readU32();
    if(meshCount>100000)throw std::runtime_error("VXMSH: excessive mesh count");
    auto floats=[&](std::size_t count){
        std::vector<float> out(count);
        for(std::size_t i=0;i<count;i++){
            const auto bits=readU32();
            std::memcpy(&out[i],&bits,4);
            if(!std::isfinite(out[i]))throw std::runtime_error("VXMSH: non-finite float");
        }
        return out;
    };
    CompiledScene result;
    result.meshes.reserve(meshCount);
    for(std::uint32_t m=0;m<meshCount;m++){
        CompiledMesh mesh;
        const auto* guid=take(36);
        mesh.brushGuid.assign(reinterpret_cast<const char*>(guid),36);
        mesh.brushGuid.resize(mesh.brushGuid.find('\0')==std::string::npos?36:mesh.brushGuid.find('\0'));
        const auto n=readU32(),indices=readU32();
        if(n>10000000||indices>30000000)throw std::runtime_error("VXMSH: excessive geometry");
        const auto b=floats(6);
        std::memcpy(mesh.bounds.data(),b.data(),sizeof(float)*6);
        mesh.positions=floats(std::size_t(n)*3);
        mesh.normals=floats(std::size_t(n)*3);
        mesh.colors=floats(std::size_t(n)*4);
        if(v2)mesh.uv0=floats(std::size_t(n)*4);
        else {
            auto legacy=floats(std::size_t(n)*2);
            mesh.uv0.assign(std::size_t(n)*4,0.f);
            for(std::size_t i=0;i<n;i++) {
                mesh.uv0[i*4]=legacy[i*2];
                mesh.uv0[i*4+1]=legacy[i*2+1];
            }
        }
        mesh.indices.reserve(indices);
        for(std::uint32_t i=0;i<indices;i++){
            auto idx=readU32();
            if(idx>=n)throw std::runtime_error("VXMSH: index out of range");
            mesh.indices.push_back(idx);
        }
        result.meshes.push_back(std::move(mesh));
    }
    if(off!=data.size())throw std::runtime_error("VXMSH: trailing bytes");
    return result;
}
} // namespace voxen::render::openbrush
