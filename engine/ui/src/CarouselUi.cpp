#include <voxen/ui/CarouselUi.hpp>
#include <imgui.h>
#include <algorithm>
#include <cstdio>

namespace voxen::ui {
void drawCarousel(CarouselState& state) {
    if (!state.visible) return;
    state.page=std::clamp(state.page,0,3);
    ImGui::SetNextWindowPos(ImVec2(8,8),ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(500,660),ImGuiCond_Always);
    constexpr ImGuiWindowFlags flags=ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;
    if (ImGui::Begin("VOXEN",nullptr,flags)) {
        if (ImGui::Button("< PREV")) state.page=(state.page+3)%4;
        ImGui::SameLine();
        ImGui::Text("%.*s",static_cast<int>(pages[state.page].size()),pages[state.page].data());
        ImGui::SameLine();
        if (ImGui::Button("NEXT >")) state.page=(state.page+1)%4;
        ImGui::Separator();
        switch(state.page) {
        case 0:
            ImGui::SeparatorText("OBJECTS / VOXENS + GLB");
            static constexpr const char* presets[]={"PLASMA","SPHERES INTERSECTIONS","MANDELBULB","STRANGE ATTRACTOR","ORGANIC SPHERE","TORUS KNOT","SPIKE BLOOM","JELLYFISH","EARTHWORM","CLOUD","METABALLS","TENTACLE CREATURE","FRACTAL PYRAMID","PHYLLOTAXIS SPIRAL","STICKMAN ECHO","CACODEMON","GOLDEEN","PBR SMILE","PULSING CUBE CLOUD","RUBIK CUBE"};
            ImGui::BeginChild("objects-unified",ImVec2(0,170),true);
            for(int i=0;i<20;++i) {
                const std::string label=std::string("[VOXEN] ")+presets[i]+"##vox"+std::to_string(i);
                if(ImGui::Selectable(label.c_str(),state.voxenPaintEnabled && state.voxenPreset==i)) {
                    state.voxenPreset=i;
                    state.voxenPaintEnabled=true;
                }
            }
            for(int i=0;i<static_cast<int>(state.glbFiles.size());++i) {
                const std::string label="[GLB] "+state.glbFiles[i]+"##model"+std::to_string(i);
                if(ImGui::Selectable(label.c_str(),!state.voxenPaintEnabled && state.glbSelected==i)) {
                    state.glbSelected=i;
                    state.voxenPaintEnabled=false;
                    state.glbPreviewRequest=true;
                }
            }
            ImGui::EndChild();
            if(state.voxenPaintEnabled) {
            ImGui::TextDisabled(state.voxenPreset==0 ? "Animated plasma field" : state.voxenPreset==1 ? "Packed intersecting spheres" : state.voxenPreset==2 ? "Power-8 fractal surface" : state.voxenPreset==3 ? "Animated 3D trajectory lines" : state.voxenPreset==4 ? "Animated deforming 3D sphere" : state.voxenPreset==5 ? "Twisting torus knot" : state.voxenPreset==6 ? "Spiky bloom" : state.voxenPreset==7 ? "Jellyfish bell" : state.voxenPreset==19 ? "Rotating Rubik cube with animated face turns" : state.voxenPreset==18 ? "Pulsating cube cloud with rotating lattice" : state.voxenPreset==17 ? "Smiling golden PBR creature with glossy eyes" : state.voxenPreset==16 ? "Animated goldfish with flowing fins and bubbles" : state.voxenPreset==15 ? "Floating one-eyed demon with charging fireball" : state.voxenPreset==14 ? "Animated stick figure with neon motion trails" : state.voxenPreset==13 ? "100-sphere evolving phyllotaxis spiral" : state.voxenPreset==12 ? "Animated fractal pyramid raymarch" : "Animated 12-segment earthworm");
            if(state.voxenPreset==2){
                ImGui::Checkbox("Experimental 3D lighting",&state.mandelbulbLighting);
                if(state.mandelbulbLighting){
                    static constexpr const char* materials[]={"IRIDESCENT","EMBER","ABYSS"};
                    ImGui::Combo("Mandelbulb material",&state.mandelbulbMaterial,materials,3);
                }
            }
            if(state.voxenPreset>=4){
                static constexpr const char* materials[]={"IRIDESCENT","EMBER","ABYSS","MATRIX","BUBBLE 3D","ANIME CEL","MANGA INK","FLOWING MARBLE","CELLULAR GLOW","FIRE","MAGMA LAVA","ASCII MORPH"};
                ImGui::Combo("Surface material",&state.voxenMaterial,materials,12);
            }
            ImGui::TextDisabled("RIGHT STICK: UP/DOWN shape | LEFT/RIGHT size");
            ImGui::TextDisabled("RIGHT TRIGGER: place one Voxen");
            ImGui::Checkbox("Paint Voxens",&state.voxenPaintEnabled);
            if(!state.voxenPaintEnabled)ImGui::TextDisabled("No Voxen selected for painting");
            ImGui::SliderFloat("Size (m)",&state.voxenScale,0.18f,0.60f,"%.2f");
            ImGui::SliderFloat("Hue",&state.voxenHue,0.f,1.f);
            ImGui::SliderFloat(state.voxenPreset>=4 ? "Deformation" : "Density",&state.voxenDensity,0.3f,2.f);
            ImGui::SliderFloat("Speed",&state.voxenSpeed,0.1f,3.f);
            ImGui::Separator();
            ImGui::TextUnformatted("FREE FLOAT / PHYSICS");
            ImGui::Checkbox("Enable motion",&state.motionEnabled);
            static constexpr const char* motionModes[]={"FREE FLOAT","SWARM / BOIDS","ROTATION"};
            ImGui::Combo("Motion mode",&state.motionMode,motionModes,3);
            if(state.motionMode==2){
                ImGui::SliderFloat("Orbit radius (m)",&state.orbitRadius,0.4f,5.f,"%.2f");
                ImGui::SliderFloat("Angular speed (rad/s)",&state.orbitSpeed,0.1f,3.f,"%.2f");
                ImGui::Checkbox("Random orbital axis",&state.orbitRandomAxis);
                ImGui::TextDisabled("Orbits follow the headset; fixed axis = vertical");
            }
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
            }
            if(!state.voxenPaintEnabled) {
            ImGui::SeparatorText("GLB SETTINGS");
            static constexpr const char* glbModes[]={"ORIGINAL","VOXEN"};
            ImGui::Combo("GLB material",&state.glbMaterialMode,glbModes,2);
            ImGui::TextDisabled("ORIGINAL: embedded materials and textures when available");
            ImGui::TextDisabled("Android/data/org.voxen.xr/files/assets");

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
            }
            break;


        case 1: {
            ImGui::SeparatorText("BACKGROUNDS / SHADERS + IMAGES + MP4");
            static constexpr const char* effects[]={"LIQUID ROOM","SOLID COLOR","MATRIX RAIN","HEPP GLOW","PHI FIELD","MANDELBROT ZOOM","SOUND DEMO","MIDNIGHT RIDE","RAINBOW SPIRAL","MOBIUS SPIRAL","THEREMIN WAVE","GEOMETRIC KALEIDOSCOPE","CHROMATIC EDGES","COSMIC PALETTE","SYNTHWAVE SUNSET","OCEAN WAVES","UNIVERSE WITHIN","CYBERCITY","VORONOI FALL","TAN GLITCH"};
            static constexpr int modeIds[]={0,1,2,3,4,5,6,7,10,11,12,13,14,15,16,17,19,20,21,22};
            ImGui::BeginChild("background-unified",ImVec2(0,235),true);
            if(ImGui::Selectable("[MR] PASSTHROUGH SOBEL",state.backgroundSourceKind==0)) {
                state.backgroundSourceKind=0;
                state.panoramaSelected=-1;
            }
            for(int i=0;i<20;++i) {
                const std::string label=std::string("[SHADER] ")+effects[i]+"##bgsh"+std::to_string(i);
                if(ImGui::Selectable(label.c_str(),state.backgroundSourceKind==3 &&
                                      state.wallShaderMode==modeIds[i])) {
                    state.backgroundSourceKind=3;
                    state.wallShaderMode=modeIds[i];
                    state.liquidRoom=true;
                    state.panoramaSelected=-1;
                }
            }
            for(int i=0;i<static_cast<int>(state.panoramaFiles.size());++i) {
                const std::string label="[IMAGE] "+state.panoramaFiles[i]+"##bgi"+std::to_string(i);
                if(ImGui::Selectable(label.c_str(),state.backgroundSourceKind==1 &&
                                      state.panoramaSelected==i)) {
                    state.backgroundSourceKind=1;
                    state.panoramaSelected=i;
                    state.videoSelected=-1;
                }
            }
            for(int i=0;i<static_cast<int>(state.videoFiles.size());++i) {
                const std::string label="[MP4] "+state.videoFiles[i]+"##bgv"+std::to_string(i);
                if(ImGui::Selectable(label.c_str(),state.backgroundSourceKind==2 &&
                                      state.videoSelected==i)) {
                    state.backgroundSourceKind=2;
                    state.videoSelected=i;
                    state.panoramaSelected=-1;
                }
            }
            ImGui::EndChild();
            if(state.backgroundSourceKind==2)
                ImGui::TextDisabled("MP4: Android Surface / OpenXR equirect2 (experimental)");
            if(state.backgroundSourceKind==1 && !state.panoramaStatus.empty())
                ImGui::TextWrapped("%s",state.panoramaStatus.c_str());
            if(state.backgroundSourceKind==1 || state.backgroundSourceKind==2) {
                ImGui::SliderFloat("Panorama yaw",&state.panoramaYawDegrees,-180.f,180.f,"%.0f deg");
                ImGui::SliderFloat("Horizontal coverage",&state.panoramaHorizontalDegrees,60.f,360.f,"%.0f deg");
                ImGui::SliderFloat("Vertical coverage",&state.panoramaVerticalDegrees,30.f,180.f,"%.0f deg");
            }
            ImGui::SeparatorText("APPLY TO");
            if(ImGui::Selectable("GLOBAL SPHERE",state.backgroundTargetSurface==-1)) {
                state.backgroundTargetSurface=-1;
            }
            ImGui::BeginChild("tracked-meta-surfaces",ImVec2(0,138),true);
            for(std::size_t i=0;i<state.detectedSurfaceIds.size();++i) {
                const auto id=state.detectedSurfaceIds[i];
                const std::string label=state.detectedSurfaceLabels[i]+"##surface"+std::to_string(id);
                if(ImGui::Selectable(label.c_str(),
                   state.backgroundTargetSurface>=0 &&
                   static_cast<std::uint64_t>(state.backgroundTargetSurface)==id)) {
                    state.backgroundTargetSurface=static_cast<std::int64_t>(id);
                    // Preserve existing wall shader rendering when a shader is
                    // selected. Image and video surface projection remain pending.
                    if(state.backgroundSourceKind==3) {
                        state.wallSourceKind=0;
                        state.liquidRoom=true;
                    } else if(state.backgroundSourceKind==1) {
                        state.wallSourceKind=1;
                        state.wallImageSelected=state.panoramaSelected;
                    } else if(state.backgroundSourceKind==2) {
                        state.wallSourceKind=2;
                        state.wallVideoSelected=state.videoSelected;
                    }
                }
            }
            ImGui::EndChild();
            if(state.detectedSurfaceIds.empty())
                ImGui::TextDisabled("No tracked Meta planes available");
            if(state.backgroundTargetSurface>=0) {
                const auto id=static_cast<std::uint64_t>(state.backgroundTargetSurface);
                const auto existing=state.surfaceMedia.find(id);
                if(existing!=state.surfaceMedia.end())
                    ImGui::Text("Assigned: %s",existing->second.filename.c_str());
                if(ImGui::Button("APPLY SELECTED MEDIA")) {
                    CarouselState::AppliedMedia media{};
                    media.kind=state.backgroundSourceKind;
                    media.shaderMode=state.wallShaderMode;
                    if(media.kind==1 && state.panoramaSelected>=0 &&
                       static_cast<std::size_t>(state.panoramaSelected)<state.panoramaFiles.size())
                        media.filename=state.panoramaFiles[state.panoramaSelected];
                    else if(media.kind==2 && state.videoSelected>=0 &&
                            static_cast<std::size_t>(state.videoSelected)<state.videoFiles.size())
                        media.filename=state.videoFiles[state.videoSelected];
                    else if(media.kind==3) media.filename="Shader";
                    else media.filename="Passthrough";
                    if(media.kind!=1 && media.kind!=2 || !media.filename.empty()) {
                        state.surfaceMedia[id]=std::move(media);
                        state.applyMediaRequest=true;
                    }
                }
                ImGui::SameLine();
                if(ImGui::Button("CLEAR ASSIGNMENT")) {
                    state.surfaceMedia.erase(id);
                    state.applyMediaRequest=true;
                }
                const auto currentAssignment=state.surfaceMedia.find(id);
                if(currentAssignment!=state.surfaceMedia.end() &&
                   currentAssignment->second.kind==2)
                    ImGui::TextDisabled("MP4 via OpenXR quad (one active surface at a time)");
            }
            if(!state.applyMediaStatus.empty())
                ImGui::TextWrapped("%s",state.applyMediaStatus.c_str());
            if(state.backgroundSourceKind==3) {
                ImGui::SliderFloat("Shader opacity",&state.liquidOpacity,0.f,0.6f);
                ImGui::Checkbox("Scene mesh wireframe",&state.showSceneMesh);
            }
            break;
        }
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
