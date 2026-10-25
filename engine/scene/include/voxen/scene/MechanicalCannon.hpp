#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

namespace voxen::scene::cannon {

// Local right-handed scene coordinates: spectator origin, forward -Z, up +Y.
// All dimensions in meters, time in seconds. No OpenXR/Vulkan handles.
struct Vec3 { float x{}, y{}, z{}; };
inline Vec3 operator+(Vec3 a, Vec3 b) { return {a.x+b.x,a.y+b.y,a.z+b.z}; }
inline Vec3 operator-(Vec3 a, Vec3 b) { return {a.x-b.x,a.y-b.y,a.z-b.z}; }
inline Vec3 operator*(Vec3 a, float s) { return {a.x*s,a.y*s,a.z*s}; }
inline Vec3 mix(Vec3 a, Vec3 b, float t) { return a*(1-t)+b*t; }
inline float smooth(float t) { t=std::clamp(t,0.f,1.f); return t*t*(3-2*t); }

enum class Part : std::uint8_t {
    Base, Turret, Barrel, Breech, PistonLeft, PistonRight,
    CoolingRing, Muzzle, ChargingCore
};
struct MechanicalPart {
    Part kind;
    Vec3 center;           // relative to cannon pivot
    Vec3 halfExtents;      // mesh box or radial/cylindrical proxy
    float emissive = 0.f;
    float recoilMeters = 0.f;
};
struct Projectile {
    Vec3 position;
    float radius;
    float energy;
    float dissolve;
    std::uint32_t material;
};
struct Frame {
    float exposure = 0.f;  // background black; emission only
    float charging = 0.f;
    float recoil = 0.f;
    float muzzleFlash = 0.f;
    float audioPulse = 0.f;
    std::vector<Projectile> projectiles;
};

struct Config {
    Vec3 cannonPosition{0.f,0.f,-10.f};
    float fireInterval = 2.6f;
    float projectileSpeed = 7.0f;
    float projectileRadius = 0.28f;
    float stopDistance = 1.5f; // projectiles never intersect spectator safety zone
    unsigned maxProjectiles = 12;
    std::uint32_t seed = 0xC4110u;
};

inline float hash01(std::uint32_t n) {
    n ^= n >> 16; n *= 0x7feb352du; n ^= n >> 15;
    n *= 0x846ca68bu; n ^= n >> 16;
    return static_cast<float>(n & 0xffffffu) / 16777215.f;
}

// Procedural construction descriptors, consumed by an eventual Vulkan mesh backend.
inline std::vector<MechanicalPart> geometry() {
    std::vector<MechanicalPart> out = {
        {Part::Base,{0,-1.10f,0},{1.55f,.22f,1.55f}},
        {Part::Turret,{0,-.55f,0},{1.18f,.55f,1.25f}},
        {Part::Barrel,{0,0,1.2f},{.43f,.43f,2.05f}},
        {Part::Breech,{0,0,-.45f},{.75f,.67f,.75f}},
        {Part::PistonLeft,{-.84f,-.35f,.75f},{.15f,.15f,1.20f}},
        {Part::PistonRight,{.84f,-.35f,.75f},{.15f,.15f,1.20f}},
        {Part::Muzzle,{0,0,3.25f},{.60f,.60f,.18f}},
        {Part::ChargingCore,{0,0,-.65f},{.34f,.34f,.34f}}
    };
    for (int i=0;i<5;++i)
        out.push_back({Part::CoolingRing,{0,0,.2f+i*.58f},{.53f,.53f,.09f}});
    return out;
}

// Stateless evaluation: supports seeking, deterministic replay and saving just config/time.
// Timeline: 0-5 reveal, 5-10 charging, from 10 repeated shots.
inline Frame evaluate(float seconds, const Config& cfg = {}) {
    Frame f;
    if (!std::isfinite(seconds) || seconds<0 || !std::isfinite(cfg.fireInterval) ||
        cfg.fireInterval<=.05f || !std::isfinite(cfg.projectileSpeed) ||
        cfg.projectileSpeed<=0 || !std::isfinite(cfg.stopDistance) ||
        cfg.stopDistance<1.f) return f;
    f.exposure=smooth(seconds/5.f);
    f.charging=seconds<5.f?0.f:(seconds<10.f?smooth((seconds-5.f)/5.f):.15f);
    if (seconds<10.f) return f;
    const float interval=cfg.fireInterval;
    const int last=static_cast<int>(std::floor((seconds-10.f)/interval));
    const float travel=std::max(0.f,10.f-cfg.stopDistance);
    const float lifetime=travel/cfg.projectileSpeed+1.15f;
    const int first=std::max(0,static_cast<int>(std::floor((seconds-10.f-lifetime)/interval)));
    const float shotAge=seconds-(10.f+last*interval);
    f.recoil=(shotAge<.5f)?(.35f*std::exp(-8.f*shotAge)*std::sin(19.f*shotAge)):0.f;
    f.muzzleFlash=std::exp(-18.f*shotAge);
    f.audioPulse=std::exp(-7.f*shotAge);
    for (int i=last;i>=first && f.projectiles.size()<cfg.maxProjectiles;--i) {
        const float age=seconds-(10.f+i*interval);
        if (age<0 || age>lifetime) continue;
        const float u=std::clamp(age*cfg.projectileSpeed/travel,0.f,1.f);
        const float spreadX=(hash01(cfg.seed+static_cast<std::uint32_t>(i)*3u)-.5f)*1.5f;
        const float spreadY=(hash01(cfg.seed+static_cast<std::uint32_t>(i)*3u+1u)-.5f)*1.0f;
        const Vec3 muzzle=cfg.cannonPosition+Vec3{0,0,3.25f};
        const Vec3 target={spreadX,spreadY,-cfg.stopDistance};
        Vec3 pos=mix(muzzle,target,smooth(u));
        pos.y+=std::sin(u*3.14159265f)*.28f;
        const float fade=age>travel/cfg.projectileSpeed?
            smooth((age-travel/cfg.projectileSpeed)/1.15f):0.f;
        f.projectiles.push_back({pos,cfg.projectileRadius*(1.f+fade*1.5f),
                                 1.f-fade,fade,static_cast<std::uint32_t>(i%4)});
    }
    return f;
}
} // namespace voxen::scene::cannon
