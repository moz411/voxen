#pragma once
#include <algorithm>
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
    std::string panoramaFile;
    float panoramaYawDegrees=0.f;
    float panoramaHorizontalDegrees=220.f;
    float panoramaVerticalDegrees=75.f;
    int sceneKind = 0; // 0 = ordinary saved scene, 1 = mechanical cannon
    int musicSource = 0, musicTrack = 0, musicFile = 0;
    float musicVolume = .45f;
    bool musicPlaying = false;
    bool liquidRoom = false;
    float liquidOpacity = .42f;
    int wallShaderMode = 0;
    bool cycleWallShaders = false;
    bool differentShaderPerSurface = false;
    float wallCycleSeconds = 30.f;
    bool motionEnabled = false;
    int motionMode = 0;
    float orbitRadius = 1.5f;
    float orbitSpeed = 0.8f;
    bool orbitRandomAxis = false;
    bool motionGravity = false;
    float motionBounce = .82f;
};
inline bool valid(const SavedRuntime& r) {
    if(r.panoramaFile.size()>128 ||
       r.panoramaFile.find('/')!=std::string::npos ||
       r.panoramaFile.find('\\')!=std::string::npos ||
       r.panoramaFile.find("..")!=std::string::npos ||
       !std::isfinite(r.panoramaYawDegrees) ||
       !std::isfinite(r.panoramaHorizontalDegrees) ||
       !std::isfinite(r.panoramaVerticalDegrees) ||
       r.panoramaYawDegrees < -180.f || r.panoramaYawDegrees > 180.f ||
       r.panoramaHorizontalDegrees < 60.f || r.panoramaHorizontalDegrees > 360.f ||
       r.panoramaVerticalDegrees < 30.f || r.panoramaVerticalDegrees > 180.f)
        return false;
    if (r.sceneKind<0 || r.sceneKind>1 || r.voxens.size()>4096 || r.musicSource<0 || r.musicSource>1 ||
        r.musicTrack<0 || r.musicTrack>2 || r.musicFile<0 ||
        !std::isfinite(r.musicVolume) || r.musicVolume<0 || r.musicVolume>1 ||
        r.motionMode<0 || r.motionMode>255 ||
        !std::isfinite(r.orbitRadius) || r.orbitRadius<0.4f || r.orbitRadius>5.f ||
        !std::isfinite(r.orbitSpeed) || r.orbitSpeed<0.1f || r.orbitSpeed>3.f ||
        !std::isfinite(r.liquidOpacity) || r.liquidOpacity<0 || r.liquidOpacity>1 ||
        r.wallShaderMode<0 || r.wallShaderMode>255 ||
        !std::isfinite(r.wallCycleSeconds) || r.wallCycleSeconds<1 || r.wallCycleSeconds>300 ||
        !std::isfinite(r.motionBounce) || r.motionBounce<0 || r.motionBounce>1) return false;
    for (const auto& v:r.voxens) {
        if (v.preset<0 || v.preset>4095 || v.material<0 || v.material>4095 ||
            !std::isfinite(v.scale) || v.scale<=0 || v.scale>10 ||
            !std::isfinite(v.hue) || !std::isfinite(v.density) || !std::isfinite(v.speed) ||
            v.hue<0 || v.hue>1 || v.density<=0 || v.density>10 || v.speed<=0 || v.speed>10) return false;
        for(float x:v.position) if(!std::isfinite(x)) return false;
        for(float x:v.velocity) if(!std::isfinite(x)) return false;
    }
    return true;
}
// Normalize live editor state before persistence. Rendering presets evolve faster
// than the snapshot format; a malformed setting must not invalidate an entire scene.
inline bool sanitizeForSave(SavedRuntime& r) {
    if (r.voxens.size()>4096) return false;
    auto finiteOr=[](float value,float fallback) {
        return std::isfinite(value) ? value : fallback;
    };
    // Invalid paths are not persisted; a missing panorama is preferable to data loss.
    if (r.panoramaFile.size()>128 || r.panoramaFile.find('/')!=std::string::npos ||
        r.panoramaFile.find('\\\\')!=std::string::npos ||
        r.panoramaFile.find("..")!=std::string::npos ||
        r.panoramaFile.find_first_of(" \\t\\r\\n")!=std::string::npos)
        r.panoramaFile.clear();
    r.panoramaYawDegrees=std::clamp(finiteOr(r.panoramaYawDegrees,0.f),-180.f,180.f);
    r.panoramaHorizontalDegrees=std::clamp(finiteOr(r.panoramaHorizontalDegrees,220.f),60.f,360.f);
    r.panoramaVerticalDegrees=std::clamp(finiteOr(r.panoramaVerticalDegrees,75.f),30.f,180.f);
    r.musicSource=std::clamp(r.musicSource,0,1);
    r.musicTrack=std::clamp(r.musicTrack,0,2);
    r.musicFile=std::max(0,r.musicFile);
    r.musicVolume=std::clamp(finiteOr(r.musicVolume,.45f),0.f,1.f);
    r.sceneKind=std::clamp(r.sceneKind,0,1);
    r.motionMode=std::clamp(r.motionMode,0,255);
    r.orbitRadius=std::clamp(finiteOr(r.orbitRadius,1.5f),.4f,5.f);
    r.orbitSpeed=std::clamp(finiteOr(r.orbitSpeed,.8f),.1f,3.f);
    r.liquidOpacity=std::clamp(finiteOr(r.liquidOpacity,.42f),0.f,1.f);
    r.wallShaderMode=std::clamp(r.wallShaderMode,0,255);
    r.wallCycleSeconds=std::clamp(finiteOr(r.wallCycleSeconds,30.f),1.f,300.f);
    r.motionBounce=std::clamp(finiteOr(r.motionBounce,.82f),0.f,1.f);
    for (auto& v:r.voxens) {
        v.preset=std::clamp(v.preset,0,4095);
        v.material=std::clamp(v.material,0,4095);
        v.scale=std::clamp(finiteOr(v.scale,.18f),.001f,10.f);
        v.hue=std::clamp(finiteOr(v.hue,.52f),0.f,1.f);
        v.density=std::clamp(finiteOr(v.density,1.f),.001f,10.f);
        v.speed=std::clamp(finiteOr(v.speed,1.f),.001f,10.f);
        for (float& x:v.position) x=finiteOr(x,0.f);
        for (float& x:v.velocity) x=finiteOr(x,0.f);
    }
    return valid(r);
}
inline std::string encodeRuntime(const SavedRuntime& r) {
    if (!valid(r)) return {};
    std::ostringstream out;
    out << std::setprecision(std::numeric_limits<float>::max_digits10);
    out << "VOXEN_RUNTIME 6\n" << r.voxens.size() << '\n';
    out << r.sceneKind << '\n';
    out << (r.panoramaFile.empty() ? "-" : r.panoramaFile) << ' '
        << r.panoramaYawDegrees << ' ' << r.panoramaHorizontalDegrees << ' '
        << r.panoramaVerticalDegrees << '\n';
    out << r.musicSource << ' ' << r.musicTrack << ' ' << r.musicFile << ' '
        << r.musicVolume << ' ' << r.musicPlaying << ' ' << r.motionEnabled << ' '
        << r.motionMode << ' ' << r.motionGravity << ' ' << r.motionBounce << ' '
        << r.orbitRadius << ' ' << r.orbitSpeed << ' ' << r.orbitRandomAxis << '\n';
    out << r.liquidRoom << ' ' << r.liquidOpacity << ' ' << r.wallShaderMode << ' '
        << r.cycleWallShaders << ' ' << r.wallCycleSeconds << ' '
        << r.differentShaderPerSurface << '\n';
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
    if(data.size()>1048576) return std::nullopt;
    std::istringstream in(data);
    std::string magic; int version=0; std::size_t count=0;
    if(!(in>>magic>>version>>count) || magic!="VOXEN_RUNTIME" || (version<1 || version>6) || count>4096)
        return std::nullopt;
    SavedRuntime r;
    if(version>=3 && !(in>>r.sceneKind))return std::nullopt;
    if(version>=4) {
        if(!(in>>r.panoramaFile>>r.panoramaYawDegrees>>
             r.panoramaHorizontalDegrees>>r.panoramaVerticalDegrees))return std::nullopt;
        if(r.panoramaFile=="-")r.panoramaFile.clear();
    }
    if(!(in>>r.musicSource>>r.musicTrack>>r.musicFile>>r.musicVolume
         >>r.musicPlaying>>r.motionEnabled>>r.motionMode>>r.motionGravity>>r.motionBounce))
        return std::nullopt;
    if(version>=5 && !(in>>r.orbitRadius>>r.orbitSpeed>>r.orbitRandomAxis))return std::nullopt;
    if(version>=2 && !(in>>r.liquidRoom>>r.liquidOpacity>>r.wallShaderMode
                       >>r.cycleWallShaders>>r.wallCycleSeconds))return std::nullopt;
    if(version>=6 && !(in>>r.differentShaderPerSurface))return std::nullopt;
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
