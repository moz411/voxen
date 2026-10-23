#pragma once
#include <array>
#include <cstdint>
#include <string_view>
#include <string>
#include <vector>

namespace voxen::ui {
// UI and XR geometry are intentionally separate: controller rays must be
// projected into panel-local UVs before forwarding mouse events to ImGui.
struct CarouselState {
    int page = 0;
    bool sceneSave = false;
    bool sceneLoad = false;
    bool scenePlace = false;
    bool sceneAnchorReady = false;
    std::size_t visiblePortalCount = 0;
    int selectedScene = 0;
    std::vector<std::string> savedSceneIds;
    std::vector<std::string> savedSceneLabels;
    std::string sceneStatus;
    int musicTrack = 0;
    bool musicPlaying = false;
    float musicVolume = 0.45f;
    bool musicReady = false;
    int musicSource = 0; // 0 procedural, 1 local MP3 files
    int musicFile = 0;
    bool musicRescan = false;
    std::vector<std::string> musicFiles;
    std::string musicError;
    bool visible = true;
    bool liquidRoom = false;
    float liquidOpacity = 0.42f;
    int maxLiquidWalls = 8;
    bool includeFloorCeiling = true;
    bool showSceneMesh = false;
    bool sceneOcclusion = false;
    float sceneDepthBias = 0.002f;
    bool cycleWallShaders = false;
    float wallCycleSeconds = 30.f;
    int wallShaderMode = 0; // 0 Liquid Room, 1 solid diagnostic
    float wallColor[3] = {0.12f,0.55f,1.0f};
    float wallSolidOpacity = 0.85f;
    float animationSpeed = 1.0f;
    int mandelbulbMaterial = 0; // 0 iridescent, 1 ember, 2 abyss
    bool mandelbulbLighting = false; // Opt-in: proven cyan shader remains default
    int voxenMaterial = 0; // independent surface material for procedural mesh shapes
    bool collideRoomMesh = true;
    int voxenPreset = 0; // Plasma or Spheres Intersections
    bool voxenPaintEnabled = true;
    float voxenScale = 0.18f;
    float voxenHue = 0.52f;
    float voxenDensity = 1.0f;
    float voxenSpeed = 1.0f;
    bool motionEnabled = false;
    int motionMode = 0; // 0 free float, 1 swarm
    float swarmRadius = 0.9f;
    float swarmStrength = 1.2f;
    float swarmSeparation = 1.0f;
    bool motionGravity = false;
    float motionSpeed = 0.35f;
    float motionBounce = 0.82f;
    float motionFloorY = 0.0f; // Tracking-space floor; user adjustable
    bool voxenClear = false;
    int voxenCount = 0;
    bool showArtworks = false;
    bool animateArtworks = true;
    int artworkMotion = 0; // 0 none, 1 breathe, 2 rotate, 3 float, 4 orbit
    float motionAmplitude = 0.25f;
    bool perfHud = true;
    float cpuMs = 0.0f;
    float gpuMs = 0.0f;
    bool gpuAvailable = false;
};
inline constexpr std::array<std::string_view, 4> pages = {
    "VOXENS","BACKGROUNDS","MUSIC","SCENES"
};
// Caller owns ImGui frame lifecycle. Draws controls into ImDrawData; no XR
// input/passthrough assumptions are hidden in this unit.
void drawCarousel(CarouselState& state);
} // namespace voxen::ui
