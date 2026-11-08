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
            if(ImGui::BeginTabBar("object-media-tabs")) {
            if(ImGui::BeginTabItem("VOXENS")) {
            for(int i=0;i<20;++i) {
                if(i==14 || i==18)continue; // Removed from catalog, keep stable IDs for saved scenes.
                const std::string label=std::string("[VOXEN] ")+presets[i]+"##vox"+std::to_string(i);
                if(ImGui::Selectable(label.c_str(),state.voxenPaintEnabled && state.voxenPreset==i)) {
                    state.voxenPreset=i;
                    state.voxenPaintEnabled=true;
                    state.particlePaintEnabled=false;
                }
            }
            ImGui::EndTabItem();
            }
            if(ImGui::BeginTabItem("GLB MODELS")) {
            for(int i=0;i<static_cast<int>(state.glbFiles.size());++i) {
                const std::string label="[GLB] "+state.glbFiles[i]+"##model"+std::to_string(i);
                if(ImGui::Selectable(label.c_str(),!state.voxenPaintEnabled && state.glbSelected==i)) {
                    state.glbSelected=i;
                    state.voxenPaintEnabled=false;
                    state.particlePaintEnabled=false;
                    state.glbPreviewRequest=true;
                }
            }
            ImGui::EndTabItem();
            }
            if(ImGui::BeginTabItem("PARTICLES")) {
                static constexpr const char* particleNames[]={"SMOKE","SNOW","STARS","EMBERS","BUBBLES","FIREFLIES","MAGIC DUST"};
                ImGui::TextDisabled("Open Brush-inspired emitters (renderer pending)");
                for(int i=0;i<7;++i) {
                    ImGui::PushID(i);
                    if(ImGui::Selectable(particleNames[i],state.particlePaintEnabled && state.particlePreset==i)) {
                        state.particlePreset=i;
                        state.particlePaintEnabled=true;
                        state.voxenPaintEnabled=true;
                    }
                    ImGui::PopID();
                }
                ImGui::Separator();
                ImGui::SliderFloat("Particle density",&state.particleDensity,0.1f,2.f);
                ImGui::SliderFloat("Particle lifetime (s)",&state.particleLifetime,0.5f,12.f);
                ImGui::SliderFloat("Particle turbulence",&state.particleTurbulence,0.f,2.f);
                ImGui::Checkbox("Particle FFT reaction",&state.particleAudioReactive);
                ImGui::TextDisabled("Procedural GPU prototype: trigger places emitter");
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
            }
            ImGui::EndChild();
            if(state.voxenPaintEnabled) {
            if(state.particlePaintEnabled)ImGui::TextDisabled("Particle emitter preview / placement");
            else ImGui::TextDisabled(state.voxenPreset==0 ? "Animated plasma field" : state.voxenPreset==1 ? "Packed intersecting spheres" : state.voxenPreset==2 ? "Power-8 fractal surface" : state.voxenPreset==3 ? "Animated 3D trajectory lines" : state.voxenPreset==4 ? "Animated deforming 3D sphere" : state.voxenPreset==5 ? "Twisting torus knot" : state.voxenPreset==6 ? "Spiky bloom" : state.voxenPreset==7 ? "Jellyfish bell" : state.voxenPreset==19 ? "Rotating Rubik cube with animated face turns" : state.voxenPreset==18 ? "Pulsating cube cloud with rotating lattice" : state.voxenPreset==17 ? "Smiling golden PBR creature with glossy eyes" : state.voxenPreset==16 ? "Animated goldfish with flowing fins and bubbles" : state.voxenPreset==15 ? "Floating one-eyed demon with charging fireball" : state.voxenPreset==14 ? "Animated stick figure with neon motion trails" : state.voxenPreset==13 ? "100-sphere evolving phyllotaxis spiral" : state.voxenPreset==12 ? "Animated fractal pyramid raymarch" : "Animated 12-segment earthworm");
            if(!state.particlePaintEnabled && (state.voxenPreset==0 || state.voxenPreset==1 ||
               state.voxenPreset>=12)) {
                static constexpr const char* volumes[]={"CUBE","SPHERE","OCTAHEDRON","CONE","CYLINDER"};
                ImGui::Combo("Material volume",&state.flatVoxenVolume,volumes,5);
                ImGui::TextDisabled("Preview and placement use the selected volume");
            }
            if(!state.particlePaintEnabled && state.voxenPreset==2){
                static constexpr const char* materials[]={"IRIDESCENT","EMBER","ABYSS"};
                ImGui::Combo("Mandelbulb material",&state.mandelbulbMaterial,materials,3);
                ImGui::Checkbox("Experimental 3D lighting",&state.mandelbulbLighting);
            }
            if(!state.particlePaintEnabled && state.voxenPreset>=4){
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
            ImGui::Checkbox("Animate next placed Voxen",&state.motionEnabled);
            ImGui::TextDisabled("Motion toggle is captured when placing each Voxen");
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
            if(ImGui::BeginTabBar("background-media-tabs")) {
            if(ImGui::BeginTabItem("PASSTHROUGH")) {
            if(ImGui::Selectable("[MR] PASSTHROUGH SOBEL",state.backgroundSourceKind==0)) {
                state.backgroundSourceKind=0;
                state.panoramaSelected=-1;
            }
            ImGui::EndTabItem();
            }
            if(ImGui::BeginTabItem("SHADERS")) {
            for(int i=0;i<20;++i) {
                const std::string label=std::string("[SHADER] ")+effects[i]+"##bgsh"+std::to_string(i);
                if(ImGui::Selectable(label.c_str(),state.backgroundSourceKind==3 &&
                                      state.wallShaderMode==modeIds[i])) {
                    state.backgroundSourceKind=3;
                    state.wallShaderMode=modeIds[i];
                    state.cycleWallShaders=false; // manual selection stops slideshow
                    state.liquidRoom=true;
                    state.panoramaSelected=-1;
                }
            }
            ImGui::Separator();
            if(ImGui::Checkbox("SLIDESHOW (all shaders)",&state.cycleWallShaders) &&
               state.cycleWallShaders) {
                state.backgroundSourceKind=3;
                state.liquidRoom=true;
                state.panoramaSelected=-1;
            }
            ImGui::Checkbox("Different shader per surface",&state.differentShaderPerSurface);
            ImGui::SliderFloat("Shader duration (seconds)",&state.wallCycleSeconds,
                               10.f,90.f,"%.0f s");
            ImGui::TextDisabled(state.differentShaderPerSurface
                ? "Each unassigned surface cycles with a stable offset"
                : "Same shader on all unassigned Meta surfaces");
            ImGui::EndTabItem();
            }
            if(ImGui::BeginTabItem("IMAGES")) {
            for(int i=0;i<static_cast<int>(state.panoramaFiles.size());++i) {
                const std::string label="[IMAGE] "+state.panoramaFiles[i]+"##bgi"+std::to_string(i);
                if(ImGui::Selectable(label.c_str(),state.backgroundSourceKind==1 &&
                                      state.panoramaSelected==i)) {
                    state.backgroundSourceKind=1;
                    state.panoramaSelected=i;
                    state.panoramaSlideshow=false;
                    state.videoSelected=-1;
                }
            }
            ImGui::Separator();
            ImGui::Checkbox("SLIDESHOW (all images)",&state.panoramaSlideshow);
            ImGui::SliderFloat("Slide duration (seconds)",&state.panoramaSlideshowSeconds,5.f,120.f,"%.0f s");
            ImGui::EndTabItem();
            }
            if(ImGui::BeginTabItem("VIDEOS")) {
            for(int i=0;i<static_cast<int>(state.videoFiles.size());++i) {
                const std::string label="[MP4] "+state.videoFiles[i]+"##bgv"+std::to_string(i);
                if(ImGui::Selectable(label.c_str(),state.backgroundSourceKind==2 &&
                                      state.videoSelected==i)) {
                    state.backgroundSourceKind=2;
                    state.videoSelected=i;
                    state.panoramaSelected=-1;
                }
            }
            ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
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
            ImGui::Separator();
            ImGui::TextUnformatted("AUDIO REACTIVE");
            ImGui::Checkbox("Enable MP3 reactions",&state.audioReactiveEnabled);
            if(state.musicSource!=1)ImGui::TextDisabled("Select MP3 FILES to analyze music");
            ImGui::BeginDisabled(!state.audioReactiveEnabled || state.musicSource!=1);
            ImGui::SliderFloat("Sensitivity",&state.audioReactiveSensitivity,0.f,12.f,"%.1fx");
            ImGui::Checkbox("Animate speed (bass)",&state.audioReactiveSpeed);
            ImGui::Checkbox("Animate hue (mid)",&state.audioReactiveHue);
            ImGui::Checkbox("Animate density (RMS)",&state.audioReactiveDensity);
            ImGui::EndDisabled();
            if(state.musicSource==1 && state.musicPlaying && !state.audioReactiveReady)
                ImGui::TextDisabled("Decoding MP3 / waiting for FFT...");
            ImGui::TextDisabled("FFT levels (before sensitivity)");
            ImGui::ProgressBar(std::clamp(state.audioReactiveBass*12.f,0.f,1.f),ImVec2(-1,0),"Bass");
            ImGui::ProgressBar(std::clamp(state.audioReactiveMid*12.f,0.f,1.f),ImVec2(-1,0),"Mid");
            ImGui::ProgressBar(std::clamp(state.audioReactiveTreble*12.f,0.f,1.f),ImVec2(-1,0),"Treble");
            ImGui::ProgressBar(std::clamp(state.audioReactiveRms*3.f,0.f,1.f),ImVec2(-1,0),"RMS");

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
                    if(ImGui::IsMouseDoubleClicked(0) && state.editMode)state.sceneLoadEdit=true;
                }
                ImGui::PopID();
            }
            ImGui::EndChild();
            // Compact buttons: fit all four actions within the scene panel.
            const float gap=ImGui::GetStyle().ItemSpacing.x;
            const float buttonWidth=std::max(64.f,(ImGui::GetContentRegionAvail().x-3.f*gap)/4.f);
            const ImVec2 buttonSize(buttonWidth,32.f);
            const bool valid=state.selectedScene>=0 &&
                static_cast<std::size_t>(state.selectedScene)<state.savedSceneIds.size();
            ImGui::BeginDisabled(!state.editMode);
            if(ImGui::Button("SAVE",buttonSize))state.sceneSave=true;
            ImGui::EndDisabled();
            ImGui::SameLine();
            ImGui::BeginDisabled(!state.editMode || !valid);
            if(ImGui::Button("LOAD",buttonSize))state.sceneLoadEdit=true;
            ImGui::EndDisabled();
            ImGui::SameLine();
            ImGui::BeginDisabled(!valid || state.sceneImmersive);
            if(ImGui::Button("DELETE",buttonSize))state.sceneDeleteConfirm=true;
            ImGui::EndDisabled();
            ImGui::SameLine();
            ImGui::BeginDisabled(!valid || state.sceneImmersive);
            if(ImGui::Button("DISPOSE",buttonSize))state.scenePlace=true;
            ImGui::EndDisabled();
            if(valid && state.scenePortalSelected) {
                ImGui::Separator();
                ImGui::TextUnformatted("SELECTED SCENE PORTAL");
                static constexpr const char* surfaceTypes[]={"DEFAULT","PICTURES","PROCEDURAL"};
                ImGui::Combo("Cylinder surface",&state.sceneSurfaceKind,surfaceTypes,3);
                if(state.sceneSurfaceKind==1) {
                    if(state.panoramaFiles.empty())ImGui::TextDisabled("No pictures available");
                    else {
                        state.scenePictureIndex=std::clamp(state.scenePictureIndex,0,
                            static_cast<int>(state.panoramaFiles.size())-1);
                        const char* preview=state.panoramaFiles[state.scenePictureIndex].c_str();
                        if(ImGui::BeginCombo("Picture",preview)) {
                            for(int i=0;i<static_cast<int>(state.panoramaFiles.size());++i)
                                if(ImGui::Selectable(state.panoramaFiles[i].c_str(),i==state.scenePictureIndex))
                                    state.scenePictureIndex=i;
                            ImGui::EndCombo();
                        }
                    }
                } else if(state.sceneSurfaceKind==2) {
                    static constexpr const char* cylinderShaders[]={
                        "Matrix Rain","Ocean Waves","Liquid Room"};
                    state.sceneProceduralMode=std::clamp(state.sceneProceduralMode,0,2);
                    ImGui::Combo("Background shader",&state.sceneProceduralMode,cylinderShaders,3);
                }
                if(ImGui::Button("APPLY SURFACE"))state.sceneSurfaceApply=true;
                ImGui::Separator();
                const char* currentAudio=state.sceneAudioIndex==0?"NONE":
                    (state.sceneAudioIndex-1<static_cast<int>(state.musicFiles.size())
                        ?state.musicFiles[state.sceneAudioIndex-1].c_str():"NONE");
                if(ImGui::BeginCombo("Approach sound",currentAudio)) {
                    if(ImGui::Selectable("NONE",state.sceneAudioIndex==0))state.sceneAudioIndex=0;
                    for(int i=0;i<static_cast<int>(state.musicFiles.size());++i)
                        if(ImGui::Selectable(state.musicFiles[i].c_str(),state.sceneAudioIndex==i+1))
                            state.sceneAudioIndex=i+1;
                    ImGui::EndCombo();
                }
                ImGui::SliderFloat("Audible radius (m)",&state.sceneAudioRadius,0.5f,10.f);
                ImGui::SliderFloat("Approach gain",&state.sceneAudioGain,0.f,1.f);
                if(ImGui::Button("APPLY SOUND"))state.sceneAudioApply=true;
                if(state.sceneHeightAvailable) {
                    ImGui::Separator();
                    ImGui::DragFloat("Scene height offset (m)",&state.sceneHeightOffset,0.1f,-100.f,100.f,"%.2f m");
                    if(ImGui::Button("APPLY HEIGHT"))state.sceneHeightApply=true;
                }
            }
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
            ImGui::TextDisabled("LOAD to edit, SAVE to update; DISPOSE then right trigger");
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
