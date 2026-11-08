#pragma once
// Open Brush .tilt ZIP reader. Requires miniz.h (mz_zip_* API).
// Decodes Dots now; other brush GUIDs remain unsupported, never misrendered.
#include <voxen/render/OpenBrushDots.hpp>
#include <miniz.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace voxen::render::openbrush {
namespace detail {
struct Cursor {
    const std::uint8_t* data;
    std::size_t size, offset=0;
    void skip(std::size_t n) {
        if (n>size-offset) throw std::runtime_error("Tilt: truncated sketch");
        offset+=n;
    }
    std::uint32_t u32() {
        if (size-offset<4) throw std::runtime_error("Tilt: truncated integer");
        auto p=data+offset;offset+=4;
        return std::uint32_t(p[0])|(std::uint32_t(p[1])<<8)|
               (std::uint32_t(p[2])<<16)|(std::uint32_t(p[3])<<24);
    }
    float f32() {
        auto bits=u32();float result;
        std::memcpy(&result,&bits,4);
        return result;
    }
};
// Extract only the BrushIndex string array, without introducing a JSON runtime.
// This accepts JSON whitespace and escaped quotes but rejects malformed arrays.
inline std::vector<std::string> brushIndex(const std::string& json) {
    auto key=json.find("\"BrushIndex\"");
    if (key==std::string::npos) throw std::runtime_error("Tilt: missing BrushIndex");
    auto colon=json.find(':',key+12);
    if(colon==std::string::npos)throw std::runtime_error("Tilt: malformed BrushIndex");
    auto i=json.find_first_not_of(" \r\n\t",colon+1);
    if(i==std::string::npos || json[i]!='[')
        throw std::runtime_error("Tilt: BrushIndex is not an array");
    ++i;
    std::vector<std::string> result;
    for (;;) {
        i=json.find_first_not_of(" \r\n\t,",i);
        if(i==std::string::npos)throw std::runtime_error("Tilt: unterminated BrushIndex");
        if(json[i]==']')break;
        if(json[i]!='"')throw std::runtime_error("Tilt: invalid BrushIndex entry");
        ++i;
        std::string value;
        bool terminated=false;
        while(i<json.size()){
            char c=json[i++];
            if(c=='"'){terminated=true;break;}
            if(c=='\\'){
                if(i>=json.size())break;
                c=json[i++];
                if(c!='"' && c!='\\' && c!='/')
                    throw std::runtime_error("Tilt: unsupported GUID escape");
            }
            value+=c;
        }
        if(!terminated || value.size()!=36 || result.size()>65535)
            throw std::runtime_error("Tilt: invalid brush GUID");
        result.push_back(value);
    }
    return result;
}
inline std::vector<std::uint8_t> zipEntry(mz_zip_archive& zip,const char* name) {
    int index=mz_zip_reader_locate_file(&zip,name,nullptr,0);
    if(index<0)throw std::runtime_error(std::string("Tilt: missing ")+name);
    mz_zip_archive_file_stat stat{};
    if(!mz_zip_reader_file_stat(&zip,index,&stat) ||
       stat.m_uncomp_size>64ull*1024*1024)
        throw std::runtime_error("Tilt: ZIP entry too large");
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(stat.m_uncomp_size));
    if(!mz_zip_reader_extract_to_mem(&zip,index,bytes.data(),bytes.size(),0))
        throw std::runtime_error("Tilt: ZIP decompression failed");
    return bytes;
}
} // namespace detail

inline std::vector<DotInstance> decodeTiltDots(const std::filesystem::path& path,
                                                float worldScale=0.025f) {
    if(!std::isfinite(worldScale) || worldScale<=0)
        throw std::runtime_error("Tilt: invalid world scale");
    mz_zip_archive zip{};
    if(!mz_zip_reader_init_file(&zip,path.string().c_str(),0))
        throw std::runtime_error("Tilt: invalid ZIP archive");
    std::vector<std::uint8_t> sketch,metadata;
    try {
        metadata=detail::zipEntry(zip,"metadata.json");
        sketch=detail::zipEntry(zip,"data.sketch");
    } catch (...) {
        mz_zip_reader_end(&zip);
        throw;
    }
    mz_zip_reader_end(&zip);
    auto brushes=detail::brushIndex(std::string(metadata.begin(),metadata.end()));
    detail::Cursor cursor{sketch.data(),sketch.size()};
    if(cursor.u32()!=0xC576A5CDu)
        throw std::runtime_error("Tilt: invalid sketch sentinel");
    auto version=cursor.u32();
    if(version!=5 && version!=6)
        throw std::runtime_error("Tilt: unsupported sketch version");
    cursor.u32(); // reserved
    cursor.skip(cursor.u32()); // header extension
    auto strokes=cursor.u32();
    if(strokes>2000000)throw std::runtime_error("Tilt: excessive stroke count");
    struct RawDot {std::array<float,3> position;std::array<float,4> color;float diameter;};
    std::vector<RawDot> raw;
    std::array<float,3> lo{INFINITY,INFINITY,INFINITY},hi{-INFINITY,-INFINITY,-INFINITY};
    std::uint64_t totalPoints=0;
    constexpr const char* dotsGuid="6a1cf9f9-032c-45ec-9b1d-a6680bee30f7";
    for(std::uint32_t stroke=0;stroke<strokes;++stroke) {
        auto brush=cursor.u32();
        if(brush>=brushes.size())throw std::runtime_error("Tilt: brush index out of range");
        std::array<float,4> color{};
        for(auto& c:color)c=cursor.f32();
        auto size=cursor.f32();
        auto strokeMask=cursor.u32(),pointMask=cursor.u32();
        if(strokeMask & ~0x1001fu || pointMask & ~3u)
            throw std::runtime_error("Tilt: unsupported stroke extension");
        float scale=1.f;
        if(strokeMask&1)cursor.skip(4);
        if(strokeMask&2)scale=cursor.f32();
        if(strokeMask&4)cursor.skip(4);
        if(strokeMask&8)cursor.skip(4);
        if(strokeMask&16)cursor.skip(4);
        if(strokeMask&0x10000)cursor.skip(cursor.u32());
        auto count=cursor.u32();
        totalPoints+=count;
        if(totalPoints>10000000)throw std::runtime_error("Tilt: too many points");
        bool dots=brushes[brush]==dotsGuid;
        for(std::uint32_t p=0;p<count;++p) {
            std::array<float,3> pos{};
            for(auto& v:pos)v=cursor.f32();
            cursor.skip(16); // quaternion
            float pressure=(pointMask&1)?cursor.f32():1.f;
            if(pointMask&2)cursor.skip(4); // timestamp
            for(int axis=0;axis<3;++axis) {
                if(!std::isfinite(pos[axis]))throw std::runtime_error("Tilt: nonfinite position");
                lo[axis]=std::min(lo[axis],pos[axis]);
                hi[axis]=std::max(hi[axis],pos[axis]);
            }
            if(dots) {
                float diameter=size*scale*worldScale*std::clamp(pressure,0.f,1.f);
                if(std::isfinite(diameter) && diameter>0)
                    raw.push_back({pos,color,diameter});
            }
        }
    }
    std::vector<DotInstance> result;
    result.reserve(raw.size());
    for(const auto& item:raw) {
        DotInstance dot;
        for(int axis=0;axis<3;++axis)
            dot.center[axis]=(item.position[axis]-(lo[axis]+hi[axis])*0.5f)*worldScale;
        dot.color=item.color;
        dot.diameterMeters=item.diameter;
        result.push_back(dot);
    }
    return result;
}
} // namespace voxen::render::openbrush
