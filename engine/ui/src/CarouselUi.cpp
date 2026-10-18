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
            static constexpr const char* effects[]={"LIQUID ROOM","SOLID COLOR","MATRIX RAIN","HEPP GLOW","PHI FIELD","MANDELBROT ZOOM"};
            ImGui::Checkbox("Different shader per wall",&state.cycleWallShaders);
            if(state.cycleWallShaders) ImGui::SliderFloat("Change every (seconds)",&state.wallCycleSeconds,10.f,90.f,"%.0f");
            else ImGui::Combo("Wall shader",&state.wallShaderMode,effects,6);
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
            static constexpr const char* sources[]={"PROCEDURAL","MP3 FILES"};
            if(ImGui::Combo("Source",&state.musicSource,sources,2)) state.musicPlaying=false;
            if(state.musicSource==1){
                if(ImGui::Button("RESCAN MUSIC FOLDER"))state.musicRescan=true;
                if(state.musicFiles.empty())ImGui::TextDisabled("No MP3 in Music folder");
                for(int i=0;i<static_cast<int>(state.musicFiles.size());++i){
                    if(ImGui::Selectable(state.musicFiles[i].c_str(),state.musicFile==i)){
                        state.musicFile=i;
                        state.musicPlaying=true;
                    }
                }
                if(!state.musicError.empty())ImGui::TextWrapped("%s",state.musicError.c_str());
            } else {
            static constexpr const char* tracks[]={"COSMOS / Drone","OCEAN / Waves","FOREST / Air & birds"};
            ImGui::Combo("Atmosphere",&state.musicTrack,tracks,3);
            }
            if(ImGui::Button(state.musicPlaying ? "PAUSE" : "PLAY"))
                state.musicPlaying=!state.musicPlaying;
            ImGui::SliderFloat("Music volume",&state.musicVolume,0.f,1.f);
            ImGui::TextDisabled(state.musicSource==0 ? "Procedural ambient loops" : "MP3 playlist / loop enabled");
            if(state.musicSource==0&&!state.musicReady)ImGui::TextDisabled("Audio output unavailable");
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
