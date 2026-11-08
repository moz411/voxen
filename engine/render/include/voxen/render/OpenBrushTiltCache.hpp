#pragma once
// Per-scene disk cache for Open Brush Dots. Versioned and invalidated by source
// size and modification time; cache files are never considered authoritative.
#include <voxen/render/OpenBrushTilt.hpp>
#include <voxen/render/OpenBrushDotsBinary.hpp>
#include <filesystem>
#include <fstream>
#include <vector>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <system_error>

namespace voxen::render::openbrush {
namespace cache_detail {
inline void write32(std::ostream& out,std::uint32_t value) {
    char b[4]={char(value),char(value>>8),char(value>>16),char(value>>24)};
    out.write(b,4);
}
inline void write64(std::ostream& out,std::uint64_t value) {
    write32(out,static_cast<std::uint32_t>(value));
    write32(out,static_cast<std::uint32_t>(value>>32));
}
inline std::uint64_t read64(const std::uint8_t* p) {
    return std::uint64_t(readU32LE(p))|(std::uint64_t(readU32LE(p+4))<<32);
}
inline std::uint64_t fnv1a(const std::string& text) {
    std::uint64_t h=14695981039346656037ull;
    for(unsigned char c:text){h^=c;h*=1099511628211ull;}
    return h;
}
inline std::string hex(std::uint64_t value) {
    constexpr char digits[]="0123456789abcdef";
    std::string out(16,'0');
    for(int i=15;i>=0;--i){out[i]=digits[value&15];value>>=4;}
    return out;
}
} // namespace cache_detail

struct TiltDotsLoad {
    std::vector<DotInstance> dots;
    bool cacheHit=false;
};
// Called on scene entry / first placement, not during global renderer startup.
// Uses unique per-file cache paths, atomic rename, and regenerates on corruption.
inline TiltDotsLoad loadTiltDotsCached(const std::filesystem::path& source,
                                       const std::filesystem::path& cacheDir) {
    namespace fs=std::filesystem;
    if(source.extension()!=".tilt" || !fs::is_regular_file(source))
        throw std::runtime_error("Tilt cache: invalid source");
    const auto size=fs::file_size(source);
    const auto modified=static_cast<std::uint64_t>(
        fs::last_write_time(source).time_since_epoch().count());
    const auto cacheName=cache_detail::hex(cache_detail::fnv1a(
        fs::absolute(source).lexically_normal().string()))+".vxdots";
    const auto cachePath=cacheDir/cacheName;
    {
        std::ifstream in(cachePath,std::ios::binary|std::ios::ate);
        if(in) {
            auto end=in.tellg();
            if(end>=40 && end<=std::streamoff(64*1024*1024)) {
                std::vector<std::uint8_t> data(static_cast<std::size_t>(end));
                in.seekg(0);
                if(in.read(reinterpret_cast<char*>(data.data()),end) &&
                   std::memcmp(data.data(),"VXTCACHE",8)==0 &&
                   readU32LE(data.data()+8)==1 &&
                   cache_detail::read64(data.data()+12)==size &&
                   cache_detail::read64(data.data()+20)==modified) {
                    try {
                        auto dots=decodeDotsBinary(data.data()+28,data.size()-28);
                        return {std::move(dots),true};
                    } catch(const std::exception&) {}
                }
            }
        }
    }
    auto dots=decodeTiltDots(source);
    try {
        fs::create_directories(cacheDir);
        const auto tmp=cachePath.string()+".tmp";
        {
            std::ofstream out(tmp,std::ios::binary|std::ios::trunc);
            if(!out)throw std::runtime_error("Tilt cache: cannot create");
            out.write("VXTCACHE",8);
            cache_detail::write32(out,1);
            cache_detail::write64(out,size);
            cache_detail::write64(out,modified);
            out.write("VXDOT001",8);
            cache_detail::write32(out,static_cast<std::uint32_t>(dots.size()));
            cache_detail::write32(out,32);
            for(const auto& dot:dots) {
                out.write(reinterpret_cast<const char*>(dot.center.data()),12);
                out.write(reinterpret_cast<const char*>(dot.color.data()),16);
                out.write(reinterpret_cast<const char*>(&dot.diameterMeters),4);
            }
            out.flush();
            if(!out)throw std::runtime_error("Tilt cache: write failed");
        }
        std::error_code ec;
        fs::rename(tmp,cachePath,ec);
        if(ec)fs::remove(tmp,ec);
    } catch(const std::exception&) {
        // Read-only/full storage must not prevent scene loading.
    }
    return {std::move(dots),false};
}
} // namespace voxen::render::openbrush
