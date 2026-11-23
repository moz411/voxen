#pragma once
#include <array>
#include <algorithm>
#include <cstdint>
#include <string_view>
#include <string>
#include <vector>
#include <unordered_map>
#include <utility>
#include <voxen/ui/SurfaceMediaEditor.hpp>
#include <voxen/ui/SurfaceMediaDrag.hpp>

namespace voxen::ui {
// UI and XR geometry are intentionally separate: controller rays must be
// projected into panel-local UVs before forwarding mouse events to ImGui.
struct CarouselState {
    int page = 3;
    bool editMode = false; // Gallery at startup; painting only in edit mode.
    bool sceneNew = false;
    bool sceneSave = false;
    bool sceneLoad = false;
    bool sceneLoadEdit = false;
    bool sceneDelete = false;
    bool sceneDeleteConfirm = false;
    bool scenePlace = false;
    bool scenePortalSelect = false;
    bool scenePortalCancel = false;
    bool scenePortalMove = false;
    bool scenePortalDelete = false;
    bool scenePortalDeleteConfirm = false;
    bool scenePortalSelected = false;
    std::string scenePortalSettingsId;
    bool sceneSurfaceApply = false;
    bool sceneAudioApply = false;
    bool sceneHeightApply = false;
    bool sceneHeightAvailable = false;
    float sceneHeightOffset = 0.f;
    int sceneSurfaceKind = 0; // 0 default, 1 picture, 2 procedural
    int scenePictureIndex = 0;
    int sceneProceduralMode = 0;
    int sceneAudioIndex = 0; // 0 disabled, otherwise 1-based MP3 index
    float sceneAudioRadius = 3.f;
    float sceneAudioGain = 0.65f;
    bool sceneEnter = false;
    bool sceneLeave = false;
    bool sceneImmersive = false;
    bool sceneAnchorReady = false;
    std::size_t visiblePortalCount = 0;
    int selectedScene = 0;
    std::vector<std::string> savedSceneIds;
    std::vector<std::string> savedSceneLabels;
    std::string sceneStatus;
    int glbSelected=0;
    int glbMaterialMode=0; // 0 ORIGINAL, 1 VOXEN
    bool glbRescan=false;
    bool glbPreviewRequest=false;
    bool glbPlaceScene=false;
    bool glbPlaceGallery=false;
    std::vector<std::string> glbFiles;
    std::string glbStatus;
    int videoSelected = -1;
    int backgroundSourceKind = 0; // 0 passthrough, 1 image, 2 video (pending render)
    int wallSourceKind = 0; // 0 shader, 1 image, 2 video (pending render)
    int wallImageSelected = -1;
    int wallVideoSelected = -1;
    // -1 = global sphere; otherwise stable detected surface ID.
    std::int64_t backgroundTargetSurface = -1;
    bool mediaPlacementEnabled=false; // controller preview and hold-to-drag
    bool mediaDragPending=false; // selected with trigger held on carousel
    std::int64_t mediaPreviewSurface=-1; // snapped Meta plane, -1 = free preview
    float mediaPreviewScale=1.f;
    int mediaToolKind=0; // 0 no selection, 1 picture, 2 video, 3 shader
    int mediaToolShaderMode=0;
    std::string mediaToolFilename;
    std::string selectedSurfaceUuid;
    std::vector<std::string> detectedSurfaceUuids;
    std::vector<std::uint64_t> detectedSurfaceIds;
    std::vector<std::string> detectedSurfaceLabels;
    SurfaceMediaEditor mediaEditor;
    SurfaceMediaDrag mediaDrag; // transient; excluded from scene snapshots
    bool applyMediaRequest=false;
    std::string applyMediaStatus;
    bool videoRescanRequest = false;
    std::vector<std::string> videoFiles;
    bool panoramaImportRequest = false;
    bool panoramaRescanRequest = false;
    int panoramaSelected = -1;
    bool panoramaSlideshow = false;
    float panoramaSlideshowSeconds = 15.f;
    float panoramaYawDegrees = 0.f;
    float panoramaHorizontalDegrees = 220.f;
    float panoramaVerticalDegrees = 75.f;
    std::vector<std::string> panoramaFiles;
    std::string panoramaStatus;
    int musicTrack = 0;
    bool musicPlaying = false;
    float musicVolume = 0.45f;
    bool musicReady = false;
    int musicSource = 0; // 0 procedural, 1 local MP3 files
    int musicFile = 0;
    bool musicRescan = false;
    std::vector<std::string> musicFiles;
    std::string musicError;
    bool audioReactiveEnabled = true;
    float audioReactiveSensitivity = 3.f;
    bool audioReactiveSpeed = true;
    bool audioReactiveHue = true;
    bool audioReactiveDensity = true;
    bool audioReactiveReady = false;
    float audioReactiveRms = 0.f;
    float audioReactiveBass = 0.f;
    float audioReactiveMid = 0.f;
    float audioReactiveTreble = 0.f;
    float audioReactiveOnset = 0.f;
    bool visible = false; // Gallery startup has no carousel.
    bool liquidRoom = false;
    float liquidOpacity = 0.42f;
    int maxLiquidWalls = 8;
    bool includeFloorCeiling = true;
    bool showSceneMesh = false;
    bool sceneOcclusion = false;
    float sceneDepthBias = 0.002f;
    bool cycleWallShaders = false;
    bool differentShaderPerSurface = false;
    float wallCycleSeconds = 30.f;
    int wallShaderMode = 0; // 0 Liquid Room, 1 solid diagnostic
    float wallColor[3] = {0.12f,0.55f,1.0f};
    float wallSolidOpacity = 0.85f;
    float animationSpeed = 1.0f;
    int mandelbulbMaterial = 0; // 0 iridescent, 1 ember, 2 abyss
    bool mandelbulbLighting = false; // Opt-in: proven cyan shader remains default
    int voxenMaterial = 0; // independent surface material for procedural mesh shapes
    bool collideRoomMesh = true;
    // Legacy 20..26 presets remain readable but are not selectable.
    bool particlePaintEnabled = false;
    int particlePreset = 0; // 0..10, IDs 27..37; 9 Chromatic Wave, 10 Electricity
    int voxenPreset = 0;
    int flatVoxenVolume = 0; // 0 cube; 1 sphere // Plasma or Spheres Intersections
    bool voxenPaintEnabled = false;
    float voxenScale = 0.18f;
    float placementDistance = 0.25f; // right-stick Y, preview and placement
    float voxenHue = 0.52f;
    float smokeRgb[3] = {0.12f,0.8f,1.f}; // Smoke RGB stored in preset-27 hue/density/speed
    int particleMotionMode=0; // 0 off, 1 oscillate, 2 rotate, 3 drift
    float particleMotionSpeed=1.f;
    float particleColorCycle=0.f; // hue revolutions per second
    int brushColorMode=0; // 0 fixed, 1..3 animated, 4 gradient along stroke
    float brushEndRgb[3]={1.f,0.2f,0.55f};

    float voxenDensity = 1.0f;
    float voxenSpeed = 1.0f;
    bool motionEnabled = false;
    int motionMode = 0; // 0 free float, 1 swarm, 2 rotation
    float orbitRadius = 1.5f;
    float orbitSpeed = 0.8f;
    bool orbitRandomAxis = false;
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
    // Selecting a media tool never modifies rendering or persisted assignments.
    void selectMedia(int kind,std::string filename,int shader=0) {
        mediaDrag.cancel();
        mediaToolKind=kind;
        mediaToolFilename=std::move(filename);
        mediaToolShaderMode=shader;
        mediaPlacementEnabled=kind>=1 && kind<=3;
        mediaDragPending=mediaPlacementEnabled;
    }
    void clearGlobalMedia() {
        backgroundSourceKind=0;liquidRoom=false;cycleWallShaders=false;
        panoramaSelected=-1;videoSelected=-1;backgroundTargetSurface=-1;
    }
    bool applySelectedGlobally() {
        if(mediaToolKind<1 || mediaToolKind>3)return false;
        int picture=-1,video=-1;
        if(mediaToolKind==1) {
            const auto it=std::find(panoramaFiles.begin(),panoramaFiles.end(),mediaToolFilename);
            if(it==panoramaFiles.end())return false;
            picture=static_cast<int>(it-panoramaFiles.begin());
        } else if(mediaToolKind==2) {
            const auto it=std::find(videoFiles.begin(),videoFiles.end(),mediaToolFilename);
            if(it==videoFiles.end())return false;
            video=static_cast<int>(it-videoFiles.begin());
        }
        backgroundTargetSurface=-1;
        liquidRoom=mediaToolKind==3;
        cycleWallShaders=false;
        backgroundSourceKind=mediaToolKind==3?0:mediaToolKind;
        panoramaSelected=picture;videoSelected=video;
        if(mediaToolKind==3)wallShaderMode=mediaToolShaderMode;
        return true;
    }
};
inline constexpr std::array<std::string_view, 4> pages = {
    "CREER","SURFACES","AUDIO","SCENES"
};
// Caller owns ImGui frame lifecycle. Draws controls into ImDrawData; no XR
// input/passthrough assumptions are hidden in this unit.
void drawCarousel(CarouselState& state);
} // namespace voxen::ui
