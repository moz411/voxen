#pragma once
#include <array>
#include <cstdint>
#include <string_view>

namespace voxen::ui {
// UI and XR geometry are intentionally separate: controller rays must be
// projected into panel-local UVs before forwarding mouse events to ImGui.
struct CarouselState {
    int page = 0;
    bool visible = true;
    bool liquidRoom = true;
    float liquidOpacity = 0.42f;
    int maxLiquidWalls = 3;
    bool showSceneMesh = false;
    bool sceneOcclusion = false;
    int wallShaderMode = 0; // 0 Liquid Room, 1 solid diagnostic
    float wallColor[3] = {0.12f,0.55f,1.0f};
    float wallSolidOpacity = 0.85f;
    float animationSpeed = 1.0f;
    bool showArtworks = true;
    bool animateArtworks = true;
    int artworkMotion = 0; // 0 none, 1 breathe, 2 rotate, 3 float, 4 orbit
    float motionAmplitude = 0.25f;
    bool perfHud = true;
    float cpuMs = 0.0f;
    float gpuMs = 0.0f;
    bool gpuAvailable = false;
};
inline constexpr std::array<std::string_view, 8> pages = {
    "COLORS","SHAPES","ALGORITHMS","SHADERS",
    "PARAMS","ANIMATIONS","SAVE / LOAD","BACKGROUNDS"
};
// Caller owns ImGui frame lifecycle. Draws controls into ImDrawData; no XR
// input/passthrough assumptions are hidden in this unit.
void drawCarousel(CarouselState& state);
} // namespace voxen::ui
