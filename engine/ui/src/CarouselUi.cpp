#include <voxen/ui/CarouselUi.hpp>
#include <imgui.h>
#include <algorithm>
#include <cstdio>
#include <cmath>

namespace voxen::ui {
void drawCarousel(CarouselState& state) {
    if (!state.visible) return;
    state.page=std::clamp(state.page,0,3);
    ImGui::SetNextWindowPos(ImVec2(8,8),ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(500,660),ImGuiCond_Always);
    constexpr ImGuiWindowFlags flags=ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;
    if (ImGui::Begin("VOXEN",nullptr,flags)) {
        const float gap=ImGui::GetStyle().ItemSpacing.x;
        const float width=std::max(50.f,(ImGui::GetContentRegionAvail().x-3.f*gap)/4.f);
        for(int i=0;i<4;++i) {
            if(i)ImGui::SameLine();
            const bool active=i==state.page;
            if(active)ImGui::PushStyleColor(ImGuiCol_Button,ImVec4(.13f,.39f,.54f,1.f));
            if(ImGui::Button(pages[i].data(),ImVec2(width,42))) {
                state.page=i;
                if(i!=1)state.mediaPlacementEnabled=false;
            }
            if(active)ImGui::PopStyleColor();
        }
        ImGui::Separator();
        switch(state.page) {
        case 0:
            state.mediaPlacementEnabled=false; // selecting brushes restores trigger painting
            ImGui::SeparatorText("CREER / OBJETS ET BRUSHES");
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
                if(ImGui::Selectable("SMOKE (Open Brush)",state.particlePaintEnabled && state.particlePreset==0)) {
                    state.particlePreset=0;
                    state.particlePaintEnabled=true;
                    state.voxenPaintEnabled=true;
                }
                if(ImGui::Selectable("STARS (Open Brush)",state.particlePaintEnabled && state.particlePreset==1)) {
                    state.particlePreset=1;
                    state.particlePaintEnabled=true;
                    state.voxenPaintEnabled=true;
                }
                if(ImGui::Selectable("FIRE (native)",state.particlePaintEnabled && state.particlePreset==2)) {
                    state.particlePreset=2;
                    state.particlePaintEnabled=true;
                    state.voxenPaintEnabled=true;
                }
                if(ImGui::Selectable("SNOW (native)",state.particlePaintEnabled && state.particlePreset==3)) {
                    state.particlePreset=3;
                    state.particlePaintEnabled=true;
                    state.voxenPaintEnabled=true;
                }
                if(ImGui::Selectable("EMBERS (native)",state.particlePaintEnabled && state.particlePreset==4)) {
                    state.particlePreset=4;
                    state.particlePaintEnabled=true;
                    state.voxenPaintEnabled=true;
                }
                if(ImGui::Selectable("BUBBLES (native)",state.particlePaintEnabled && state.particlePreset==5)) {
                    state.particlePreset=5;
                    state.particlePaintEnabled=true;
                    state.voxenPaintEnabled=true;
                }
                if(ImGui::Selectable("LIGHTNING (native)",state.particlePaintEnabled && state.particlePreset==6)) {
                    state.particlePreset=6;state.particlePaintEnabled=true;state.voxenPaintEnabled=true;
                }
                if(ImGui::Selectable("NEON TRAILS (native)",state.particlePaintEnabled && state.particlePreset==7)) {
                    state.particlePreset=7;state.particlePaintEnabled=true;state.voxenPaintEnabled=true;
                }
                if(ImGui::Selectable("MAGIC DUST (native)",state.particlePaintEnabled && state.particlePreset==8)) {
                    state.particlePreset=8;state.particlePaintEnabled=true;state.voxenPaintEnabled=true;
                }
                if(ImGui::Selectable("CHROMATIC WAVE (native)",state.particlePaintEnabled && state.particlePreset==9)) {
                    state.particlePreset=9;state.particlePaintEnabled=true;state.voxenPaintEnabled=true;
                }
                if(ImGui::Selectable("CHAOS CITY (random cubes)",state.particlePaintEnabled && state.particlePreset==12)) {
                    state.particlePreset=12;state.particlePaintEnabled=true;state.voxenPaintEnabled=true;
                }
                if(ImGui::Selectable("SUN (incandescent)",state.particlePaintEnabled && state.particlePreset==11)) {
                    state.particlePreset=11;state.particlePaintEnabled=true;state.voxenPaintEnabled=true;
                }
                if(ImGui::Selectable("ELECTRICITY (native)",state.particlePaintEnabled && state.particlePreset==10)) {
                    state.particlePreset=10;state.particlePaintEnabled=true;state.voxenPaintEnabled=true;
                }
                ImGui::TextDisabled("Hold right trigger and draw a continuous stroke");
                ImGui::TextDisabled("Native GPU particles / light ribbons");
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
            }
            ImGui::EndChild();
            if(state.voxenPaintEnabled) {
            if(state.particlePaintEnabled)ImGui::TextDisabled("Native particle stroke (hold trigger)");
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
            ImGui::TextDisabled("RIGHT STICK: LEFT/RIGHT size (hold) | UP/DOWN distance");
            ImGui::TextDisabled("X: undo one stroke/placement per press (history)");
            ImGui::Text("Placement distance: %.2f m",state.placementDistance);
            ImGui::TextDisabled(state.particlePaintEnabled ? "RIGHT TRIGGER: draw particles" : "RIGHT TRIGGER: place one Voxen");
            if(state.particlePaintEnabled) {
                static constexpr const char* colorModes[]={
                    "FIXED COLOR","CYCLE SLOW","CYCLE MEDIUM","CYCLE FAST",
                    "EVOLVING: START TO END OF STROKE"};
                ImGui::Combo("Color variation",&state.brushColorMode,colorModes,5);
                state.particleColorCycle=state.brushColorMode==1 ? 0.5f :
                    state.brushColorMode==2 ? 1.f :
                    state.brushColorMode==3 ? 2.f : 0.f;
                if(state.brushColorMode==4)
                    ImGui::ColorEdit3("End-of-stroke color",state.brushEndRgb);
            } else if(ImGui::Button("Evolve Voxen color")) {
                state.voxenHue=std::fmod(state.voxenHue+0.125f,1.f);
            }
            ImGui::SliderFloat("Size (m)",&state.voxenScale,0.18f,
                state.particlePaintEnabled?6.f:0.60f,"%.2f",ImGuiSliderFlags_Logarithmic);
            if(ImGui::CollapsingHeader("MOUVEMENT / ANIMATION")) {
            if(state.particlePaintEnabled) {
                static constexpr const char* animations[]={"NONE","OSCILLATE","ROTATE","DRIFT","ORBIT WHOLE SCENE"};
                ImGui::Combo("Brush motion",&state.particleMotionMode,animations,5);
                if(state.particleMotionMode==4)
                    ImGui::TextDisabled("Rotate each new stroke around its own centroid");
                ImGui::SliderFloat("Brush motion speed",&state.particleMotionSpeed,0.1f,4.f,"%.2f");
                ImGui::TextDisabled("Motion and color settings apply only to the next stroke.");
                if(ImGui::CollapsingHeader("PALETTE RGB AVANCEE")) {
                ImGui::ColorPicker3("Particle RGB",state.smokeRgb,
                    ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_DisplayRGB |
                    ImGuiColorEditFlags_InputRGB | ImGuiColorEditFlags_NoSidePreview);
                ImGui::ColorButton("Current color",ImVec4(state.smokeRgb[0],
                    state.smokeRgb[1],state.smokeRgb[2],1.f));
                static constexpr float palette[][3]={
                    {1.f,1.f,1.f},{0.12f,0.8f,1.f},{1.f,0.2f,0.4f},
                    {1.f,0.65f,0.12f},{0.25f,1.f,0.45f},{0.65f,0.25f,1.f}};
                for(int i=0;i<6;++i) {
                    ImGui::PushID(i);
                    if(ImGui::ColorButton("##swatch",ImVec4(palette[i][0],
                        palette[i][1],palette[i][2],1.f),
                        ImGuiColorEditFlags_NoTooltip,ImVec2(42,32)))
                        std::copy(palette[i],palette[i]+3,state.smokeRgb);
                    if(i!=5)ImGui::SameLine();
                    ImGui::PopID();
                }
                }
            } else ImGui::SliderFloat("Hue",&state.voxenHue,0.f,1.f);
            ImGui::SliderFloat(state.voxenPreset>=4 ? "Deformation" : "Density",&state.voxenDensity,0.3f,2.f);
            ImGui::SliderFloat("Speed",&state.voxenSpeed,0.1f,3.f);
            if(!state.particlePaintEnabled) {
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
            }
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
            ImGui::SeparatorText("MEDIAS A DEPOSER");
            static constexpr const char* effects[]={"LIQUID ROOM","SOLID COLOR","MATRIX RAIN","HEPP GLOW","PHI FIELD","MANDELBROT ZOOM","SOUND DEMO","MIDNIGHT RIDE","RAINBOW SPIRAL","MOBIUS SPIRAL","THEREMIN WAVE","GEOMETRIC KALEIDOSCOPE","CHROMATIC EDGES","COSMIC PALETTE","SYNTHWAVE SUNSET","OCEAN WAVES","UNIVERSE WITHIN","CYBERCITY","VORONOI FALL","TAN GLITCH"};
            static constexpr int modeIds[]={0,1,2,3,4,5,6,7,10,11,12,13,14,15,16,17,19,20,21,22};
            ImGui::BeginChild("background-unified",ImVec2(0,235),true);
            if(ImGui::BeginTabBar("background-media-tabs")) {
            if(ImGui::BeginTabItem("PASSTHROUGH")) {
                ImGui::TextWrapped("Sobel est le fond par defaut des surfaces sans media.");
                ImGui::TextDisabled("Pour restaurer Sobel, effacer la surface ou le fond global.");
            ImGui::EndTabItem();
            }
            if(ImGui::BeginTabItem("SHADERS")) {
            for(int i=0;i<20;++i) {
                const std::string label=std::string("[SHADER] ")+effects[i]+"##bgsh"+std::to_string(i);
                if(ImGui::Selectable(label.c_str(),state.mediaToolKind==3 &&
                                      state.mediaToolShaderMode==modeIds[i])) {
                    state.selectMedia(3,"Shader",modeIds[i]);
                }
            }
            ImGui::EndTabItem();
            }
            if(ImGui::BeginTabItem("IMAGES")) {
            for(int i=0;i<static_cast<int>(state.panoramaFiles.size());++i) {
                const std::string label="[IMAGE] "+state.panoramaFiles[i]+"##bgi"+std::to_string(i);
                if(ImGui::Selectable(label.c_str(),state.mediaToolKind==1 &&
                                      state.mediaToolFilename==state.panoramaFiles[i])) {
                    state.selectMedia(1,state.panoramaFiles[i]);
                }
            }
            ImGui::EndTabItem();
            }
            if(ImGui::BeginTabItem("VIDEOS")) {
            for(int i=0;i<static_cast<int>(state.videoFiles.size());++i) {
                const std::string label="[MP4] "+state.videoFiles[i]+"##bgv"+std::to_string(i);
                if(ImGui::Selectable(label.c_str(),state.mediaToolKind==2 &&
                                      state.mediaToolFilename==state.videoFiles[i])) {
                    state.selectMedia(2,state.videoFiles[i]);
                }
            }
            ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
            }
            ImGui::EndChild();
            ImGui::Text("Selection : %s",state.mediaToolKind==0?"aucune":
                state.mediaToolFilename.c_str());
            ImGui::SeparatorText("PLACEMENT SPATIAL");
            ImGui::TextDisabled("Maintenir la gachette sur un media et deplacer l'icone vers une surface.");
            ImGui::TextDisabled("Le plan surligne est la cible du depot.");
            ImGui::TextDisabled("Tirer vers soi : decrocher | relacher : confirmer.");
            ImGui::Text("Distance %.2fm / taille %.2fx",state.placementDistance,state.mediaPreviewScale);
            if(state.mediaDrag.active && state.mediaDrag.snapped)
                ImGui::TextColored(ImVec4(.25f,.85f,.8f,1.f),"SURFACE ACCROCHEE - relacher pour fixer");
            else if(state.mediaDrag.active)ImGui::TextDisabled("Glissement libre : pousser jusqu'au plan");
            else ImGui::TextDisabled("Maintenir la gachette pour saisir l'icone");
            if(!state.applyMediaStatus.empty())ImGui::TextWrapped("%s",state.applyMediaStatus.c_str());
            ImGui::BeginDisabled(!state.mediaEditor.canUndo());
            if(ImGui::Button("ANNULER"))state.mediaEditor.undo();
            ImGui::EndDisabled();
            ImGui::SameLine();
            ImGui::BeginDisabled(!state.mediaEditor.canRedo());
            if(ImGui::Button("RETABLIR"))state.mediaEditor.redo();
            ImGui::EndDisabled();
            if(ImGui::Button("TERMINER LE PLACEMENT",ImVec2(-1,34)))
                {state.mediaPlacementEnabled=false;state.mediaDrag.cancel();}
            if(ImGui::CollapsingHeader("ENVIRONNEMENT GLOBAL (explicite)")) {
                ImGui::TextDisabled("Independant des affectations par surface.");
                if(ImGui::Button("RETOUR PASSTHROUGH SOBEL",ImVec2(-1,36))) {
                    state.clearGlobalMedia();
                }
                ImGui::BeginDisabled(state.mediaToolKind==0);
                if(ImGui::Button("APPLIQUER LA SELECTION AU GLOBAL",ImVec2(-1,36)))
                    state.applySelectedGlobally();
                ImGui::EndDisabled();
                if(state.liquidRoom) {
                    ImGui::Checkbox("Diaporama shaders GLOBAL",&state.cycleWallShaders);
                    ImGui::Checkbox("Decalage par surface",&state.differentShaderPerSurface);
                    if(state.cycleWallShaders)ImGui::SliderFloat("Duree (s)",&state.wallCycleSeconds,10.f,90.f);
                    ImGui::SliderFloat("Opacite shader",&state.liquidOpacity,0.f,0.6f);
                }
                if(state.backgroundSourceKind==1 || state.backgroundSourceKind==2) {
                    ImGui::SliderFloat("Yaw panorama",&state.panoramaYawDegrees,-180.f,180.f);
                    ImGui::SliderFloat("Couverture H",&state.panoramaHorizontalDegrees,60.f,360.f);
                    ImGui::SliderFloat("Couverture V",&state.panoramaVerticalDegrees,30.f,180.f);
                    if(state.backgroundSourceKind==1) {
                        ImGui::Checkbox("Diaporama images",&state.panoramaSlideshow);
                        if(state.panoramaSlideshow)ImGui::SliderFloat("Duree diapo (s)",
                            &state.panoramaSlideshowSeconds,5.f,120.f);
                    }
                }
            }
            if(ImGui::CollapsingHeader("OPTIONS AVANCEES")) {
                ImGui::Checkbox("Maillage scene",&state.showSceneMesh);
                ImGui::Checkbox("Occlusion monde reel",&state.sceneOcclusion);
                ImGui::SliderFloat("Biais profondeur",&state.sceneDepthBias,0.f,0.02f);
            }
            if(!state.applyMediaStatus.empty())ImGui::TextWrapped("%s",state.applyMediaStatus.c_str());
            ImGui::TextDisabled("Videos : une seule surface MP4 active simultanement");
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
            if(ImGui::CollapsingHeader("AUDIO REACTIVE / FFT")) {
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
            }

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
            // Compact buttons: fit all five actions within the scene panel.
            const float gap=ImGui::GetStyle().ItemSpacing.x;
            const float buttonWidth=std::max(52.f,(ImGui::GetContentRegionAvail().x-4.f*gap)/5.f);
            const ImVec2 buttonSize(buttonWidth,32.f);
            const bool valid=state.selectedScene>=0 &&
                static_cast<std::size_t>(state.selectedScene)<state.savedSceneIds.size();
            ImGui::BeginDisabled(!state.editMode);
            if(ImGui::Button("NEW",buttonSize))state.sceneNew=true;
            ImGui::SameLine();
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
