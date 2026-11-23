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
    std::uint32_t strokeId=0; // one ID per trigger-held stroke
    int brushMotion=0, brushColorMode=0;
    float brushMotionSpeed=1.f, brushColorCycle=0.f;
    std::array<float,3> brushEndRgb{1.f,.2f,.55f};
};
struct SavedSurfaceMedia {
    std::string uuid; // Meta spatial-entity UUID, stable across scene reloads
    int kind=0; // 0 local passthrough/Sobel, 1 picture, 2 video, 3 shader
    std::string filename;
    int shaderMode=0;
};
struct SavedRuntime {
    std::vector<SavedVoxen> voxens;
    std::vector<SavedSurfaceMedia> surfaceMedia;
    std::string panoramaFile;
    std::string videoFile;
    int backgroundSourceKind=0;
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
    int particleMotionMode=0;
    float particleMotionSpeed=1.f;
    float particleColorCycle=0.f;
};
inline bool valid(const SavedRuntime& r) {
    if(r.surfaceMedia.size()>256)return false;
    for(const auto& media:r.surfaceMedia) {
        if(media.uuid.empty() || media.uuid.size()>80 ||
           media.uuid.find_first_not_of("0123456789abcdefABCDEF-")!=std::string::npos ||
           media.kind<0 || media.kind>3 || media.shaderMode<0 || media.shaderMode>255 ||
           media.filename.size()>128 || media.filename.find('/')!=std::string::npos ||
           media.filename.find('\\')!=std::string::npos ||
           media.filename.find("..")!=std::string::npos ||
           media.filename.find_first_of(" \t\r\n")!=std::string::npos ||
           (media.kind!=3 && media.filename.empty()))return false;
    }
    if(r.backgroundSourceKind<0 || r.backgroundSourceKind>2 ||
       r.videoFile.size()>128 || r.videoFile.find('/')!=std::string::npos ||
       r.videoFile.find('\\')!=std::string::npos ||
       r.videoFile.find("..")!=std::string::npos ||
       r.videoFile.find_first_of(" \t\r\n")!=std::string::npos)return false;
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
        r.particleMotionMode<0 || r.particleMotionMode>4 ||
        !std::isfinite(r.particleMotionSpeed) || r.particleMotionSpeed<0.1f || r.particleMotionSpeed>4.f ||
        !std::isfinite(r.particleColorCycle) || r.particleColorCycle<0.f || r.particleColorCycle>2.f ||
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
        if(v.brushMotion<0 || v.brushMotion>4 || v.brushColorMode<0 || v.brushColorMode>4 ||
           !std::isfinite(v.brushMotionSpeed) || v.brushMotionSpeed<.1f || v.brushMotionSpeed>4.f ||
           !std::isfinite(v.brushColorCycle) || v.brushColorCycle<0.f || v.brushColorCycle>2.f)
            return false;
        for(float c:v.brushEndRgb)if(!std::isfinite(c) || c<0.f || c>1.f)return false;
    }
    return true;
}
// Normalize live editor state before persistence. Rendering presets evolve faster
// than the snapshot format; a malformed setting must not invalidate an entire scene.
inline bool sanitizeForSave(SavedRuntime& r) {
    if (r.voxens.size()>4096) return false;
    if(r.surfaceMedia.size()>256)return false;
    auto finiteOr=[](float value,float fallback) {
        return std::isfinite(value) ? value : fallback;
    };
    if(r.videoFile.size()>128 || r.videoFile.find('/')!=std::string::npos ||
       r.videoFile.find('\\')!=std::string::npos ||
       r.videoFile.find("..")!=std::string::npos ||
       r.videoFile.find_first_of(" \t\r\n")!=std::string::npos)
        r.videoFile.clear();
    r.backgroundSourceKind=std::clamp(r.backgroundSourceKind,0,2);
    // Invalid paths are not persisted; a missing panorama is preferable to data loss.
    if (r.panoramaFile.size()>128 || r.panoramaFile.find('/')!=std::string::npos ||
        r.panoramaFile.find('\\')!=std::string::npos ||
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
    r.particleMotionMode=std::clamp(r.particleMotionMode,0,4);
    r.particleMotionSpeed=std::clamp(finiteOr(r.particleMotionSpeed,1.f),.1f,4.f);
    r.particleColorCycle=std::clamp(finiteOr(r.particleColorCycle,0.f),0.f,2.f);
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
        v.brushMotion=std::clamp(v.brushMotion,0,4);
        v.brushColorMode=std::clamp(v.brushColorMode,0,4);
        v.brushMotionSpeed=std::clamp(finiteOr(v.brushMotionSpeed,1.f),.1f,4.f);
        v.brushColorCycle=std::clamp(finiteOr(v.brushColorCycle,0.f),0.f,2.f);
        for(float& c:v.brushEndRgb)c=std::clamp(finiteOr(c,1.f),0.f,1.f);
    }
    return valid(r);
}
inline std::string encodeRuntime(const SavedRuntime& r) {
    if (!valid(r)) return {};
    std::ostringstream out;
    out << std::setprecision(std::numeric_limits<float>::max_digits10);
    out << "VOXEN_RUNTIME 10\n" << r.voxens.size() << '\n';
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
    out << r.backgroundSourceKind << ' ' << (r.videoFile.empty()?"-":r.videoFile) << '\n';
    out << r.particleMotionMode << ' ' << r.particleMotionSpeed << ' ' << r.particleColorCycle << '\n';
    for(const auto& v:r.voxens) {
        out << v.preset << ' ' << v.material << ' ' << v.scale << ' ' << v.hue
            << ' ' << v.density << ' ' << v.speed;
        for(float x:v.position) out << ' ' << x;
        for(float x:v.velocity) out << ' ' << x;
        out << ' ' << v.strokeId << ' ' << v.brushMotion << ' ' << v.brushMotionSpeed
            << ' ' << v.brushColorMode << ' ' << v.brushColorCycle;
        for(float x:v.brushEndRgb)out << ' ' << x;
        out << '\n';
    }
    out << r.surfaceMedia.size() << '\n';
    for(const auto& media:r.surfaceMedia)
        out << media.uuid << ' ' << media.kind << ' '
            << (media.filename.empty()?"-":media.filename) << ' '
            << media.shaderMode << '\n';
    return out.str();
}
inline std::optional<SavedRuntime> decodeRuntime(const std::string& data) {
    if(data.size()>1048576) return std::nullopt;
    std::istringstream in(data);
    std::string magic; int version=0; std::size_t count=0;
    if(!(in>>magic>>version>>count) || magic!="VOXEN_RUNTIME" || (version<1 || version>10) || count>4096)
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
    if(version>=7) {
        if(!(in>>r.backgroundSourceKind>>r.videoFile))return std::nullopt;
        if(r.videoFile=="-")r.videoFile.clear();
    } else r.backgroundSourceKind=r.panoramaFile.empty()?0:1;
    if(version>=8 && !(in>>r.particleMotionMode>>r.particleMotionSpeed>>r.particleColorCycle))
        return std::nullopt;
    for(std::size_t i=0;i<count;++i) {
        SavedVoxen v;
        if(!(in>>v.preset>>v.material>>v.scale>>v.hue>>v.density>>v.speed))
            return std::nullopt;
        for(float& x:v.position) if(!(in>>x)) return std::nullopt;
        for(float& x:v.velocity) if(!(in>>x)) return std::nullopt;
        if(version>=9) {
            if(!(in>>v.strokeId>>v.brushMotion>>v.brushMotionSpeed>>v.brushColorMode>>v.brushColorCycle))
                return std::nullopt;
            for(float& x:v.brushEndRgb)if(!(in>>x))return std::nullopt;
        } else if(v.preset>=27 && v.preset<=37) {
            // Legacy v8 scenes had scene-global animation. Preserve that visual
            // behavior while assigning independent defaults to each old stroke.
            v.brushMotion=r.particleMotionMode;
            v.brushMotionSpeed=r.particleMotionSpeed;
            v.brushColorCycle=r.particleColorCycle;
            v.brushColorMode=r.particleColorCycle>0.f?2:0;
        }
        r.voxens.push_back(v);
    }
    if(version>=10) {
        std::size_t mediaCount=0;
        if(!(in>>mediaCount) || mediaCount>256)return std::nullopt;
        for(std::size_t i=0;i<mediaCount;++i) {
            SavedSurfaceMedia media;
            if(!(in>>media.uuid>>media.kind>>media.filename>>media.shaderMode))return std::nullopt;
            if(media.filename=="-")media.filename.clear();
            r.surfaceMedia.push_back(std::move(media));
        }
    }
    in >> std::ws;
    if(!in.eof() || !valid(r)) return std::nullopt;
    return r;
}
} // namespace voxen::gallery
