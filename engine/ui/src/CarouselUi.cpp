#include <voxen/ui/CarouselUi.hpp>
#include <imgui.h>
#include <algorithm>
#include <cstdio>

namespace voxen::ui {
void drawCarousel(CarouselState& state) {
    if (!state.visible) return;
    state.page=std::clamp(state.page,0,8);
    ImGui::SetNextWindowPos(ImVec2(8,8),ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(500,660),ImGuiCond_Always);
    constexpr ImGuiWindowFlags flags=ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;
    if (ImGui::Begin("VOXEN",nullptr,flags)) {
        if (ImGui::Button("< PREV")) state.page=(state.page+8)%9;
        ImGui::SameLine();
        ImGui::Text("%.*s",static_cast<int>(pages[state.page].size()),pages[state.page].data());
        ImGui::SameLine();
        if (ImGui::Button("NEXT >")) state.page=(state.page+1)%9;
        ImGui::Separator();
        switch(state.page) {
        case 3:
            ImGui::TextUnformatted("NATIVE VULKAN MATERIALS");
            ImGui::Checkbox("Render procedural artworks",&state.showArtworks);
            ImGui::TextDisabled("Current: Neon / Organic / Fractal");
            break;
        case 4:
            ImGui::SliderFloat("Time multiplier",&state.animationSpeed,0.1f,3.0f);
            ImGui::Checkbox("Performance metrics",&state.perfHud);
            break;
        case 5: {
            ImGui::Checkbox("Animate artworks",&state.animateArtworks);
            static constexpr const char* motions[]={"NONE","BREATHE","ROTATE","FLOAT","ORBIT"};
            ImGui::Combo("Motion",&state.artworkMotion,motions,5);
            ImGui::SliderFloat("Animation speed",&state.animationSpeed,0.1f,3.0f);
            ImGui::SliderFloat("Amplitude",&state.motionAmplitude,0.0f,0.5f);
            ImGui::TextDisabled("Applies to the 4 native demo artworks");
            break;
        }
        case 7:
            ImGui::Checkbox("Wall effect",&state.liquidRoom);
            static constexpr const char* effects[]={"LIQUID ROOM","SOLID COLOR","MATRIX RAIN","HEPP GLOW","PHI FIELD"};
            ImGui::Combo("Wall shader",&state.wallShaderMode,effects,5);
            if(state.wallShaderMode==1) {
                ImGui::ColorEdit3("Wall color",state.wallColor);
                ImGui::SliderFloat("Solid opacity",&state.wallSolidOpacity,0.1f,1.0f);
            } else {
                ImGui::SliderFloat("Opacity",&state.liquidOpacity,0.0f,0.6f);
            }
            ImGui::SliderInt("Max walls",&state.maxLiquidWalls,1,8);
            ImGui::Checkbox("Floor & ceiling",&state.includeFloorCeiling);
            if(state.wallShaderMode==3)ImGui::TextDisabled("Hepp Maccoy 2019 / CC BY-NC-SA 3.0");
            ImGui::Checkbox("Scene mesh wireframe",&state.showSceneMesh);
            ImGui::Checkbox("Occlude walls with scene mesh",&state.sceneOcclusion);
            if(state.sceneOcclusion) ImGui::SliderFloat("Occlusion bias",&state.sceneDepthBias,0.0f,0.01f,"%.4f");
            ImGui::TextDisabled("Quest fast mode: nearest tracked walls");
            break;
        case 8: {
            ImGui::TextUnformatted("AMBIENT SOUND");
            static constexpr const char* tracks[]={"COSMOS / Drone","OCEAN / Waves","FOREST / Air & birds"};
            ImGui::Combo("Atmosphere",&state.musicTrack,tracks,3);
            if(ImGui::Button(state.musicPlaying ? "PAUSE" : "PLAY"))
                state.musicPlaying=!state.musicPlaying;
            ImGui::SliderFloat("Music volume",&state.musicVolume,0.f,1.f);
            ImGui::TextDisabled("Procedural loops - no audio files needed");
            if(!state.musicReady)ImGui::TextDisabled("Audio output unavailable");
            break;
        }
        default:
            ImGui::TextDisabled("Legacy controls: port pending");
            break;
        }
        ImGui::Separator();
        if(state.perfHud) {
            ImGui::Text("APP CPU  %.2f ms",state.cpuMs);
            if(state.gpuAvailable) ImGui::Text("GPU  %.2f ms",state.gpuMs);
            else ImGui::TextDisabled("GPU  N/A");
        }
    }
    ImGui::End();
}
} // namespace voxen::ui
