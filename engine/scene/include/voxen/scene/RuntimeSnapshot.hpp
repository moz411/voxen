#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <limits>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

namespace voxen::gallery {
struct SavedVoxen {
    int preset = 0;
    std::array<float,3> position{}, velocity{};
    float scale = .18f, hue = .52f, density = 1.f, speed = 1.f;
    int material = 0;
};
struct SavedRuntime {
    std::vector<SavedVoxen> voxens;
    int musicSource = 0, musicTrack = 0, musicFile = 0;
    float musicVolume = .45f;
    bool musicPlaying = false;
    bool liquidRoom = false;
    float liquidOpacity = .42f;
    int wallShaderMode = 0;
    bool cycleWallShaders = false;
    float wallCycleSeconds = 30.f;
    bool motionEnabled = false;
    int motionMode = 0;
    bool motionGravity = false;
    float motionBounce = .82f;
};
inline bool valid(const SavedRuntime& r) {
    if (r.voxens.size()>256 || r.musicSource<0 || r.musicSource>1 ||
        r.musicTrack<0 || r.musicTrack>2 || r.musicFile<0 ||
        !std::isfinite(r.musicVolume) || r.musicVolume<0 || r.musicVolume>1 ||
        r.motionMode<0 || r.motionMode>1 ||
        !std::isfinite(r.liquidOpacity) || r.liquidOpacity<0 || r.liquidOpacity>1 ||
        r.wallShaderMode<0 || r.wallShaderMode>6 ||
        !std::isfinite(r.wallCycleSeconds) || r.wallCycleSeconds<1 || r.wallCycleSeconds>300 ||
        !std::isfinite(r.motionBounce) || r.motionBounce<0 || r.motionBounce>1) return false;
    for (const auto& v:r.voxens) {
        if (v.preset<0 || v.preset>7 || v.material<0 || v.material>3 ||
            !std::isfinite(v.scale) || v.scale<=0 || v.scale>10 ||
            !std::isfinite(v.hue) || !std::isfinite(v.density) || !std::isfinite(v.speed) ||
            v.hue<0 || v.hue>1 || v.density<=0 || v.density>10 || v.speed<=0 || v.speed>10) return false;
        for(float x:v.position) if(!std::isfinite(x)) return false;
        for(float x:v.velocity) if(!std::isfinite(x)) return false;
    }
    return true;
}
inline std::string encodeRuntime(const SavedRuntime& r) {
    if (!valid(r)) return {};
    std::ostringstream out;
    out << std::setprecision(std::numeric_limits<float>::max_digits10);
    out << "VOXEN_RUNTIME 2\n" << r.voxens.size() << '\n';
    out << r.musicSource << ' ' << r.musicTrack << ' ' << r.musicFile << ' '
        << r.musicVolume << ' ' << r.musicPlaying << ' ' << r.motionEnabled << ' '
        << r.motionMode << ' ' << r.motionGravity << ' ' << r.motionBounce << '\n';
    out << r.liquidRoom << ' ' << r.liquidOpacity << ' ' << r.wallShaderMode << ' '
        << r.cycleWallShaders << ' ' << r.wallCycleSeconds << '\n';
    for(const auto& v:r.voxens) {
        out << v.preset << ' ' << v.material << ' ' << v.scale << ' ' << v.hue
            << ' ' << v.density << ' ' << v.speed;
        for(float x:v.position) out << ' ' << x;
        for(float x:v.velocity) out << ' ' << x;
        out << '\n';
    }
    return out.str();
}
inline std::optional<SavedRuntime> decodeRuntime(const std::string& data) {
    if(data.size()>65536) return std::nullopt;
    std::istringstream in(data);
    std::string magic; int version=0; std::size_t count=0;
    if(!(in>>magic>>version>>count) || magic!="VOXEN_RUNTIME" || (version!=1 && version!=2) || count>256)
        return std::nullopt;
    SavedRuntime r;
    if(!(in>>r.musicSource>>r.musicTrack>>r.musicFile>>r.musicVolume
         >>r.musicPlaying>>r.motionEnabled>>r.motionMode>>r.motionGravity>>r.motionBounce))
        return std::nullopt;
    if(version==2 && !(in>>r.liquidRoom>>r.liquidOpacity>>r.wallShaderMode
                       >>r.cycleWallShaders>>r.wallCycleSeconds))return std::nullopt;
    for(std::size_t i=0;i<count;++i) {
        SavedVoxen v;
        if(!(in>>v.preset>>v.material>>v.scale>>v.hue>>v.density>>v.speed))
            return std::nullopt;
        for(float& x:v.position) if(!(in>>x)) return std::nullopt;
        for(float& x:v.velocity) if(!(in>>x)) return std::nullopt;
        r.voxens.push_back(v);
    }
    in >> std::ws;
    if(!in.eof() || !valid(r)) return std::nullopt;
    return r;
}
} // namespace voxen::gallery
