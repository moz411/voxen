#include <voxen/ui/CarouselUi.hpp>
#include <imgui.h>
#include <algorithm>
#include <cstdio>

namespace voxen::ui {
void drawCarousel(CarouselState& state) {
    if (!state.visible) return;
    state.page=std::clamp(state.page,0,4);
    ImGui::SetNextWindowPos(ImVec2(8,8),ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(500,660),ImGuiCond_Always);
    constexpr ImGuiWindowFlags flags=ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;
    if (ImGui::Begin("VOXEN",nullptr,flags)) {
        if (ImGui::Button("< PREV")) state.page=(state.page+4)%5;
        ImGui::SameLine();
        ImGui::Text("%.*s",static_cast<int>(pages[state.page].size()),pages[state.page].data());
        ImGui::SameLine();
        if (ImGui::Button("NEXT >")) state.page=(state.page+1)%5;
        ImGui::Separator();
        switch(state.page) {
        case 0:
            ImGui::TextUnformatted("VOXENS / PROCEDURAL PRESETS");
            ImGui::Separator();
            static constexpr const char* presets[]={"PLASMA","SPHERES INTERSECTIONS","MANDELBULB","STRANGE ATTRACTOR","ORGANIC SPHERE","TORUS KNOT","SPIKE BLOOM","JELLYFISH","EARTHWORM","CLOUD","METABALLS"};
            if(ImGui::Combo("Voxen preset",&state.voxenPreset,presets,11))state.voxenPaintEnabled=true;
            ImGui::TextDisabled(state.voxenPreset==0 ? "Animated plasma field" : state.voxenPreset==1 ? "Packed intersecting spheres" : state.voxenPreset==2 ? "Power-8 fractal surface" : state.voxenPreset==3 ? "Animated 3D trajectory lines" : state.voxenPreset==4 ? "Animated deforming 3D sphere" : state.voxenPreset==5 ? "Twisting torus knot" : state.voxenPreset==6 ? "Spiky bloom" : state.voxenPreset==7 ? "Jellyfish bell" : "Animated 12-segment earthworm");
            if(state.voxenPreset==2){
                ImGui::Checkbox("Experimental 3D lighting",&state.mandelbulbLighting);
                if(state.mandelbulbLighting){
                    static constexpr const char* materials[]={"IRIDESCENT","EMBER","ABYSS"};
                    ImGui::Combo("Mandelbulb material",&state.mandelbulbMaterial,materials,3);
                }
            }
            if(state.voxenPreset>=4){
                static constexpr const char* materials[]={"IRIDESCENT","EMBER","ABYSS","MATRIX","BUBBLE 3D","ANIME CEL","MANGA INK","FLOWING MARBLE","CELLULAR GLOW"};
                ImGui::Combo("Surface material",&state.voxenMaterial,materials,9);
            }
            ImGui::TextDisabled("RIGHT STICK: UP/DOWN shape | LEFT/RIGHT size");
            ImGui::TextDisabled("RIGHT TRIGGER: place one Voxen");
            ImGui::Checkbox("Paint Voxens",&state.voxenPaintEnabled);
            if(!state.voxenPaintEnabled)ImGui::TextDisabled("No Voxen selected for painting");
            ImGui::SliderFloat("Size (m)",&state.voxenScale,0.06f,0.60f,"%.2f");
            ImGui::SliderFloat("Hue",&state.voxenHue,0.f,1.f);
            ImGui::SliderFloat(state.voxenPreset>=4 ? "Deformation" : "Density",&state.voxenDensity,0.3f,2.f);
            ImGui::SliderFloat("Speed",&state.voxenSpeed,0.1f,3.f);
            ImGui::Separator();
            ImGui::TextUnformatted("FREE FLOAT / PHYSICS");
            ImGui::Checkbox("Enable motion",&state.motionEnabled);
            static constexpr const char* motionModes[]={"FREE FLOAT","SWARM / BOIDS"};
            ImGui::Combo("Motion mode",&state.motionMode,motionModes,2);
            if(state.motionMode==1){
                ImGui::SliderFloat("Swarm radius (m)",&state.swarmRadius,0.25f,2.5f,"%.2f");
                ImGui::SliderFloat("Swarm strength",&state.swarmStrength,0.1f,4.f,"%.2f");
                ImGui::SliderFloat("Separation",&state.swarmSeparation,0.1f,4.f,"%.2f");
                ImGui::TextDisabled("Local cohesion + alignment + separation");
            }
            ImGui::Checkbox("Gravity",&state.motionGravity);
            ImGui::Checkbox("Wall + ceiling collisions",&state.collideRoomMesh);
            ImGui::SliderFloat("Initial speed (m/s)",&state.motionSpeed,0.05f,1.5f,"%.2f");
            ImGui::SliderFloat("Bounce",&state.motionBounce,0.f,1.f,"%.2f");
            ImGui::TextDisabled("Floor: headset STAGE; ceiling: scanned mesh");
            ImGui::TextDisabled("No floor collision until STAGE is available");
            ImGui::Text("Voxens: %d / 256",state.voxenCount);
            if(ImGui::Button("CLEAR VOXENS"))state.voxenClear=true;
            ImGui::TextDisabled("Right trigger paints in space");
            ImGui::TextDisabled("Audio / deformation / trajectories: next steps");
            break;


        case 1:
            ImGui::SeparatorText("PANORAMAS / LOCAL PHOTOS");
            if (ImGui::Button("IMPORT JPEG / PNG")) state.panoramaImportRequest=true;
            ImGui::SameLine();
            if (ImGui::Button("RESCAN PHOTOS")) state.panoramaRescanRequest=true;
            ImGui::BeginChild("panorama-list",ImVec2(0,100),true);
            for (int i=0;i<static_cast<int>(state.panoramaFiles.size());++i)
                if(ImGui::Selectable(state.panoramaFiles[i].c_str(),state.panoramaSelected==i))
                    state.panoramaSelected=i;
            ImGui::EndChild();
            if(!state.panoramaStatus.empty()) ImGui::TextWrapped("%s",state.panoramaStatus.c_str());
            ImGui::TextDisabled("Photo projection: not connected yet");
            ImGui::Separator();
            ImGui::Checkbox("Wall effect",&state.liquidRoom);
            static constexpr const char* effects[]={"LIQUID ROOM","SOLID COLOR","MATRIX RAIN","HEPP GLOW","PHI FIELD","MANDELBROT ZOOM","SOUND DEMO","MIDNIGHT RIDE","SEASCAPE","OCTGRAMS"};
            ImGui::Checkbox("Different shader per wall",&state.cycleWallShaders);
            if(state.cycleWallShaders) ImGui::SliderFloat("Change every (seconds)",&state.wallCycleSeconds,10.f,90.f,"%.0f");
            else ImGui::Combo("Wall shader",&state.wallShaderMode,effects,10);
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
        case 2: {
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
        case 4: {
            ImGui::TextUnformatted("GLB / LOCAL ASSETS");
            static constexpr const char* glbModes[]={"ORIGINAL","VOXEN"};
            ImGui::Combo("GLB material",&state.glbMaterialMode,glbModes,2);
            ImGui::TextDisabled("ORIGINAL: base color; textures pending");
            ImGui::TextDisabled("Android/data/org.voxen.xr/files/assets");
            if(ImGui::Button("RESCAN ASSETS"))state.glbRescan=true;
            ImGui::BeginChild("glb-list",ImVec2(0,280),true);
            for(int i=0;i<static_cast<int>(state.glbFiles.size());++i)
                if(ImGui::Selectable(state.glbFiles[i].c_str(),state.glbSelected==i)) {
                    state.glbSelected=i;
                    state.voxenPaintEnabled=false;
                    state.glbPreviewRequest=true;
                }
            ImGui::EndChild();
            if(state.glbFiles.empty())ImGui::TextDisabled("No .glb files found");
            if(state.glbFiles.empty())ImGui::BeginDisabled();
            if(ImGui::Button("PREVIEW",ImVec2(140,44))) {
                state.glbPreviewRequest=true;
                state.voxenPaintEnabled=false;
            }
            ImGui::SameLine();
            if(ImGui::Button("PLACE IN SCENE",ImVec2(140,44)))state.glbPlaceScene=true;
            ImGui::SameLine();
            if(ImGui::Button("PLACE IN GALLERY",ImVec2(220,44)))state.glbPlaceGallery=true;
            if(state.glbFiles.empty())ImGui::EndDisabled();
            ImGui::TextWrapped("%s",state.glbStatus.c_str());
            ImGui::TextDisabled("Preview follows right controller");
            ImGui::TextDisabled("RIGHT STICK LEFT/RIGHT: resize (default 0.1)");
            ImGui::TextDisabled("RIGHT TRIGGER: place preview");
            break;
        }
        case 3: {
            ImGui::TextUnformatted("SAVED SCENES");
            ImGui::BeginChild("scene-list",ImVec2(0,360),true);
            for(std::size_t i=0;i<state.savedSceneIds.size();++i) {
                const auto& label=i<state.savedSceneLabels.size()?state.savedSceneLabels[i]:state.savedSceneIds[i];
                ImGui::PushID(static_cast<int>(i));
                if(ImGui::Selectable(label.c_str(),state.selectedScene==static_cast<int>(i))) {
                    state.selectedScene=static_cast<int>(i);
                    state.sceneDeleteConfirm=false;
                    if(ImGui::IsMouseDoubleClicked(0))state.sceneLoad=true;
                }
                ImGui::PopID();
            }
            ImGui::EndChild();
            if(!state.editMode)ImGui::BeginDisabled();
            if(ImGui::Button("SAVE",ImVec2(140,44)))state.sceneSave=true;
            ImGui::SameLine();
            if(!state.editMode || state.savedSceneIds.empty())ImGui::BeginDisabled();
            if(ImGui::Button("CHARGER",ImVec2(140,44)))state.sceneLoadEdit=true;
            if(!state.editMode || state.savedSceneIds.empty())ImGui::EndDisabled();
            if(!state.editMode)ImGui::EndDisabled();
            ImGui::SameLine();
            const bool valid=state.selectedScene>=0 &&
                static_cast<std::size_t>(state.selectedScene)<state.savedSceneIds.size();
            if(!valid || state.sceneImmersive)ImGui::BeginDisabled();
            if(ImGui::Button("DELETE",ImVec2(140,44)))state.sceneDeleteConfirm=true;
            if(!valid || state.sceneImmersive)ImGui::EndDisabled();
            ImGui::SameLine();
            if(!valid || state.sceneImmersive)ImGui::BeginDisabled();
            if(ImGui::Button("DISPOSE",ImVec2(140,44)))state.scenePlace=true;
            if(!valid || state.sceneImmersive)ImGui::EndDisabled();
            if(state.sceneDeleteConfirm) {
                ImGui::TextUnformatted("Delete selected saved scene?");
                if(ImGui::Button("CONFIRM DELETE")) {
                    state.sceneDelete=true;
                    state.sceneDeleteConfirm=false;
                }
                ImGui::SameLine();
                if(ImGui::Button("CANCEL"))state.sceneDeleteConfirm=false;
            }
            if(!state.sceneStatus.empty())ImGui::TextWrapped("%s",state.sceneStatus.c_str());
            ImGui::TextDisabled("Double-click scene to load; DISPOSE then right trigger");
            if(!state.sceneAnchorReady)
                ImGui::TextDisabled("Waiting for localized Meta room anchor");
            if(state.sceneImmersive && ImGui::Button("EXIT SCENE"))state.sceneLeave=true;
            break;
        }
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
