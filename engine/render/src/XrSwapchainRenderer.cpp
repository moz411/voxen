#include <voxen/render/XrSwapchainRenderer.hpp>
#include <voxen/render/MandelbulbMesh.hpp>
#ifdef VOXEN_WITH_GLB_IMPORT
#include <voxen/render/PanoramaImage.hpp>
#endif
#ifdef VOXEN_WITH_GLB_IMPORT
#include <voxen/scene/GlbMeshDecoder.hpp>
#endif
#ifdef VOXEN_WITH_IMGUI
#include <imgui.h>
#include <backends/imgui_impl_vulkan.h>
#include <voxen/ui/CarouselUi.hpp>
#include <voxen/ui/SpatialPanelInput.hpp>
#endif

#ifdef _WIN32
#include <windows.h>
#include <unknwn.h>
#endif
#include <openxr/openxr_platform.h>

#ifdef __ANDROID__
#include <android/log.h>
#else
#include <cstdio>
#endif

#include "torusKnotVert.spv.hpp"
#include "spikeBloomVert.spv.hpp"
#include "jellyfishVert.spv.hpp"
#include "earthwormVert.spv.hpp"
#include "metaballsVert.spv.hpp"
#include "cloudVert.spv.hpp"
#include "cloudFrag.spv.hpp"
#include "organicSphereVert.spv.hpp"
#include "organicSphereFrag.spv.hpp"
#include "cubeVert.spv.hpp"
#include "voxenPlasmaFrag.spv.hpp"
#include "voxenSpheresFrag.spv.hpp"
#include "voxenMandelbulbFrag.spv.hpp"
#include "voxenVolumeVert.spv.hpp"
#include "voxenAttractorVert.spv.hpp"
#include "voxenAttractorFrag.spv.hpp"
#include "fractalVert.spv.hpp"
#include "laserFrag.spv.hpp"
#include "liquidRoomFrag.spv.hpp"
#include "solidWallFrag.spv.hpp"
#include "mandelbulbLitVert.spv.hpp"
#include "mandelbulbLitFrag.spv.hpp"
#include "glbOriginalVert.spv.hpp"
#include "glbOriginalFrag.spv.hpp"
#include "glbTexturedVert.spv.hpp"
#include "glbTexturedFrag.spv.hpp"
#include "meshWireVert.spv.hpp"
#include "meshWireFrag.spv.hpp"
#include "meshDepthVert.spv.hpp"
#include "meshDepthFrag.spv.hpp"
#include "matrixRainFrag.spv.hpp"
#include "heppGlowFrag.spv.hpp"
#include "phiFieldFrag.spv.hpp"
#include "soundDemoFrag.spv.hpp"
#include "midnightRideFrag.spv.hpp"
#include "seascapeFrag.spv.hpp"
#include "octgramsFrag.spv.hpp"
#include "mandelbrotZoomFrag.spv.hpp"
#include "carouselFrag.spv.hpp"
#include "galleryPortalFrag.spv.hpp"
#include "mechanicalCannonFrag.spv.hpp"
#include "imguiPanelFrag.spv.hpp"
#include "panoramaVert.spv.hpp"
#include "panoramaFrag.spv.hpp"

#include <algorithm>
#include <future>
#include <cmath>
#include <array>
#include <cstring>
#include <cfloat>
#include <cctype>
#include <stdexcept>
#include <string>
#include <vector>

namespace voxen::render {
namespace {

void checkXr(XrResult result, const char* operation) {
    if (XR_FAILED(result)) {
        throw std::runtime_error(std::string(operation) + " failed with XrResult " + std::to_string(result));
    }
}

void checkVk(VkResult result, const char* operation) {
    if (result != VK_SUCCESS) {
        throw std::runtime_error(std::string(operation) + " failed with VkResult " + std::to_string(result));
    }
}

} // namespace


#ifdef VOXEN_WITH_GLB_IMPORT
void XrSwapchainRenderer::rescanGlbAssets() {
    glbAssetPaths_.clear();
    glbAssetNames_.clear();
    try {
        std::filesystem::create_directories(glbAssetRoot_);
        for(const auto& item:std::filesystem::directory_iterator(glbAssetRoot_)) {
            if(!item.is_regular_file() || item.is_symlink())continue;
            auto extension=item.path().extension().string();
            std::transform(extension.begin(),extension.end(),extension.begin(),
                           [](unsigned char v){return static_cast<char>(std::tolower(v));});
            if(extension!=".glb")continue;
            glbAssetPaths_.push_back(item.path());
        }
        std::sort(glbAssetPaths_.begin(),glbAssetPaths_.end());
        for(const auto& path:glbAssetPaths_)
            glbAssetNames_.push_back(path.filename().string());
    } catch(const std::exception&) {
        glbAssetPaths_.clear();
        glbAssetNames_.clear();
    }
}
#endif

void XrSwapchainRenderer::rescanPanoramaAssets() {
#ifdef VOXEN_WITH_IMGUI
    const int oldIndex=imguiCarousel_.panoramaSelected;
    const std::string oldName=(oldIndex>=0 && static_cast<std::size_t>(oldIndex)<panoramaNames_.size())
        ? panoramaNames_[oldIndex] : std::string{};
#endif
    panoramaNames_.clear();
    try {
        if(panoramaRoot_.empty())return;
        std::filesystem::create_directories(panoramaRoot_);
        for(const auto& item:std::filesystem::directory_iterator(panoramaRoot_)) {
            if(!item.is_regular_file() || item.is_symlink())continue;
            auto ext=item.path().extension().string();
            std::transform(ext.begin(),ext.end(),ext.begin(),
                [](unsigned char c){return static_cast<char>(std::tolower(c));});
            if(ext==".jpg" || ext==".jpeg" || ext==".png")
                panoramaNames_.push_back(item.path().filename().string());
        }
        std::sort(panoramaNames_.begin(),panoramaNames_.end());
#ifdef VOXEN_WITH_IMGUI
        const auto found=std::find(panoramaNames_.begin(),panoramaNames_.end(),oldName);
        imguiCarousel_.panoramaSelected=oldName.empty() || found==panoramaNames_.end()
            ? -1 : static_cast<int>(found-panoramaNames_.begin());
        panoramaDecodedSelection_=-2;
#endif
    } catch(const std::exception&) {
        panoramaNames_.clear();
    }
}

void XrSwapchainRenderer::configureSceneStorage(const std::filesystem::path& root) {
    sceneRepository_ = std::make_unique<voxen::gallery::SceneRepository>(root);
#ifdef __ANDROID__
    panoramaRoot_="/sdcard/Android/data/org.voxen.xr/files/Backgrounds";
#else
    panoramaRoot_=root / "Backgrounds";
#endif
    rescanPanoramaAssets();
    portalManager_ = std::make_unique<voxen::gallery::PortalManager>(*sceneRepository_);
    // Seed a real repository scene so gallery placement and walk-in work unchanged.
    if(!sceneRepository_->loadSceneMetadata(mechanicalCannonSceneId_)) {
        voxen::gallery::SavedRuntime cannon;
        cannon.sceneKind=1;
        cannon.liquidRoom=false;
        auto record=voxen::gallery::SceneRepository::newScene("Canon mecanique");
        record.id=mechanicalCannonSceneId_;
        record.preview.materialPreset=4;
        record.preview.boundingRadiusMeters=2.5f;
        sceneRepository_->saveScene(record,voxen::gallery::encodeRuntime(cannon));
    }
    refreshSceneIndex();
    selectedSceneIndex_ = 0;
#ifdef VOXEN_WITH_GLB_IMPORT
    // Debug-only static asset; requires explicit file in app-private storage.
#ifdef __ANDROID__
    glbAssetRoot_="/sdcard/Android/data/org.voxen.xr/files/assets";
#else
    glbAssetRoot_=root / "assets";
#endif
    rescanGlbAssets();
#endif
}

void XrSwapchainRenderer::refreshSceneIndex() {
    sceneIds_=sceneRepository_?sceneRepository_->listScenes():
        std::vector<voxen::gallery::SceneId>{};
    sceneLabels_.clear();
    scenePreviewCache_.clear();
    sceneLabels_.reserve(sceneIds_.size());
    for(const auto& id:sceneIds_) {
        const auto metadata=sceneRepository_->loadSceneMetadata(id);
        if(metadata) {
            scenePreviewCache_[id]=metadata->preview;
            sceneLabels_.push_back(metadata->displayName+
                "  [proxy " + std::to_string(metadata->preview.boundingRadiusMeters).substr(0,4) + "m]");
        } else {
            sceneLabels_.push_back(id);
        }
    }
}

void XrSwapchainRenderer::setLocalizedGalleryRoom(
    std::optional<voxen::gallery::LocalizedRoom> room) {
    if(portalManager_) portalManager_->setLocalization(std::move(room));
}

std::vector<voxen::gallery::VisiblePortal> XrSwapchainRenderer::visibleGalleryPortals() const {
    return portalManager_ ? portalManager_->visible()
                          : std::vector<voxen::gallery::VisiblePortal>{};
}

std::optional<voxen::gallery::FloorHit> XrSwapchainRenderer::galleryFloorHit() const {
    if(!controllerTracked_[1])return std::nullopt;
    const auto& pose=controllerPoses_[1];
    const auto& q=pose.orientation;
    const std::array<float,3> direction{
        -2.f*(q.x*q.z+q.w*q.y),
        -2.f*(q.y*q.z-q.w*q.x),
        -(1.f-2.f*(q.x*q.x+q.y*q.y))};
    return voxen::gallery::intersectTrackedFloor(
        environmentSurfaces_.surfaces(),
        {pose.position.x,pose.position.y,pose.position.z},direction);
}

bool XrSwapchainRenderer::placeGalleryPortalFromRightController() {
    if(!portalManager_ || !portalManager_->localized() ||
       selectedSceneIndex_<0 ||
       static_cast<std::size_t>(selectedSceneIndex_)>=sceneIds_.size())return false;
    const auto hit=galleryFloorHit();
    if(!hit)return false;
    const auto id=portalManager_->place(sceneIds_[selectedSceneIndex_],*hit);
    if(id)selectedPortalId_=*id;
    return id.has_value();
}

bool XrSwapchainRenderer::selectGalleryPortalFromRightController() {
    if(!portalManager_ || !portalManager_->localized())return false;
    const auto hit=galleryFloorHit();
    if(!hit)return false;
    selectedPortalId_=portalManager_->nearest(*hit);
    return selectedPortalId_.has_value();
}

bool XrSwapchainRenderer::moveSelectedGalleryPortalFromRightController() {
    if(!portalManager_ || !selectedPortalId_)return false;
    const auto hit=galleryFloorHit();
    return hit && portalManager_->move(*selectedPortalId_,*hit);
}

bool XrSwapchainRenderer::deleteSelectedGalleryPortal() {
    if(!portalManager_ || !selectedPortalId_)return false;
    if(!portalManager_->erase(*selectedPortalId_))return false;
    selectedPortalId_.reset();
    return true;
}

std::vector<voxen::gallery::SceneId> XrSwapchainRenderer::savedSceneIds() const {
    return sceneRepository_ ? sceneRepository_->listScenes()
                            : std::vector<voxen::gallery::SceneId>{};
}

void XrSwapchainRenderer::requestGalleryDelete() {
    // Delete only the current ray-hovered gallery sphere; never delete a
    // previous selection or an immersive-scene object by accident.
    if(editingMode() || immersiveSceneActive_ || galleryDragging_ ||
       pendingGalleryFloorAction_!=0 || !portalManager_ || !portalManager_->localized())
        return;
    if(!galleryHoveredId_) {
#ifdef __ANDROID__
        __android_log_print(ANDROID_LOG_INFO,"VoxenGallery","B delete: no hovered sphere");
#endif
        return;
    }
    const auto id=*galleryHoveredId_;
    const bool ok=portalManager_->erase(id);
#ifdef __ANDROID__
    __android_log_print(ok?ANDROID_LOG_INFO:ANDROID_LOG_WARN,"VoxenGallery",
                        "B delete hovered sphere: %s",ok?"success":"failed");
#endif
    if(ok) {
        selectedPortalId_.reset();
        galleryHoveredId_.reset();
        galleryDeleteArmedUntil_={};
#ifdef VOXEN_WITH_IMGUI
        imguiCarousel_.sceneStatus="Sphere deleted; saved scene retained";
#endif
    }
}

void XrSwapchainRenderer::setEditingMode(bool enabled) noexcept {
#ifdef VOXEN_WITH_IMGUI
    if(immersiveSceneActive_)return;
    if(enabled==imguiCarousel_.editMode) {
        imguiCarousel_.visible=enabled;
        return;
    }
    if(enabled) {
        galleryDragging_=false;
        selectedPortalId_.reset();
        galleryHoveredId_.reset();
        galleryDeleteArmedUntil_={};
        imguiCarousel_=editorSettings_;
        imguiCarousel_.editMode=true;
        imguiCarousel_.visible=true;
        voxens_.swap(editorVoxens_);
        pendingGalleryFloorAction_=0;
    } else {
        editorSettings_=imguiCarousel_;
        editorSettings_.editMode=true;
        editorSettings_.visible=true;
        voxens_.swap(editorVoxens_);
        imguiCarousel_.editMode=false;
        imguiCarousel_.visible=false;
        // Keep the panorama in editorSettings_ for the next edit session,
        // but Gallery must always render the live Sobel passthrough.
        imguiCarousel_.panoramaSelected=-1;
        panoramaFadeActive_=false;
        ambientMusic_.configure(0,false,0.f);
        mp3Library_.update(-1,false,0.f);
    }
    lastVoxenValid_=false;
    panelAimed_=false;
    galleryEntryCandidate_.reset();
    galleryEntryArmed_=true;
#else
    (void)enabled;
#endif
}

void XrSwapchainRenderer::updateGalleryWalkIn(const XrVector3f& headPosition) {
    if(editingMode() || pendingGalleryFloorAction_!=0 || galleryDragging_)return;
    if(!portalManager_ || !portalManager_->localized()) {
        galleryEntryCandidate_.reset();
        galleryExitSince_={};
        return;
    }
    // Enter/exit using the headset's actual 3D position relative to a sphere.
    // Hysteresis prevents repeated scene loads at the boundary.
    const auto portals=portalManager_->visible();
    if(immersiveSceneActive_) {
        if(!immersivePortalId_)return; // explicit editing session
        const auto it=std::find_if(portals.begin(),portals.end(),
            [&](const voxen::gallery::VisiblePortal& p){return p.id==*immersivePortalId_;});
        if(it==portals.end()) { leaveImmersiveScene(); return; }
        const auto& p=it->localPose.positionMeters;
        const float dx=headPosition.x-p[0],dy=headPosition.y-p[1],dz=headPosition.z-p[2];
        const float radius=0.30f*std::clamp(it->scale,0.1f,4.f);
        if(dx*dx+dy*dy+dz*dz<=radius*radius) {
            galleryExitSince_={};
            return;
        }
        const auto now=std::chrono::steady_clock::now();
        if(galleryExitSince_.time_since_epoch().count()==0) {
            galleryExitSince_=now;
            return;
        }
        if(now-galleryExitSince_>=std::chrono::milliseconds(400))
            leaveImmersiveScene();
        return;
    }
    const voxen::gallery::VisiblePortal* inside=nullptr;
    float bestDistance2=100000.f;
    for(const auto& portal:portals) {
        const auto& p=portal.localPose.positionMeters;
        const float dx=headPosition.x-p[0],dy=headPosition.y-p[1],dz=headPosition.z-p[2];
        const float d2=dx*dx+dy*dy+dz*dz;
        const float radius=0.25f*std::clamp(portal.scale,0.1f,4.f);
        if(d2<radius*radius && d2<bestDistance2) {
            inside=&portal;
            bestDistance2=d2;
        }
    }
    if(!inside) {
        galleryEntryCandidate_.reset();
        galleryEntryArmed_=true;
        return;
    }
    if(!galleryEntryArmed_)return;
    const auto now=std::chrono::steady_clock::now();
    if(!galleryEntryCandidate_ || *galleryEntryCandidate_!=inside->id) {
        galleryEntryCandidate_=inside->id;
        galleryEntrySince_=now;
        return;
    }
    if(now-galleryEntrySince_<std::chrono::milliseconds(850))return;
    // Loading happens once after a stable dwell, never every frame.
    galleryEntryArmed_=false;
    const auto sceneId=inside->sceneId;
    galleryEntryCandidate_.reset();
    if(enterImmersiveScene(sceneId)) {
        immersivePortalId_=inside->id;
        galleryExitSince_={};
    } else {
#ifdef VOXEN_WITH_IMGUI
        imguiCarousel_.sceneStatus="Automatic portal entry failed";
#endif
    }
}

bool XrSwapchainRenderer::enterImmersiveScene(const voxen::gallery::SceneId& id) {
    if(immersiveSceneActive_ || !sceneRepository_) return false;
    // Validate and parse before touching the current live scene.
    auto saved=sceneRepository_->loadScene(id);
    auto immersiveSnapshot=saved ? voxen::gallery::decodeRuntime(saved->second) : std::nullopt;
    if(!immersiveSnapshot)return false;
    // Enter with only the requested scene instances resident.
    std::vector<PlasmaVoxen>{}.swap(voxens_);
    if(!loadSavedScene(id)) return false;
    mechanicalCannonPreview_=immersiveSnapshot->sceneKind==1;
    mechanicalCannonStart_=std::chrono::steady_clock::now();
#ifdef VOXEN_WITH_IMGUI
    imguiCarousel_.editMode=false;
    imguiCarousel_.visible=false;
    imguiCarousel_.musicPlaying=immersiveSnapshot->musicPlaying;
    panelAimed_=false;
#endif
    immersiveSceneActive_=true;
    return true;
}

bool XrSwapchainRenderer::leaveImmersiveScene() {
    if(!immersiveSceneActive_) return false;
#ifdef VOXEN_WITH_IMGUI
    imguiCarousel_.musicPlaying=false;
#endif
    ambientMusic_.configure(0,false,0.f);
    mp3Library_.update(-1,false,0.f);
    // Release the per-scene CPU instance storage. Global pipelines, mesh
    // caches and descriptor allocations remain resident for now.
    std::vector<PlasmaVoxen>{}.swap(voxens_);
    lastVoxenValid_=false;
    physicsAccumulator_=0.f;
    immersiveSceneActive_=false;
    mechanicalCannonPreview_=false;
#ifdef VOXEN_WITH_IMGUI
    imguiCarousel_.liquidRoom=false;
    imguiCarousel_.musicPlaying=false;
    // Gallery always returns to live Meta passthrough with cyan Sobel edges.
    // The next render tick releases the panorama texture and discards any
    // in-flight asynchronous decode by comparing the selected index.
    imguiCarousel_.panoramaSelected=-1;
    panoramaFadeActive_=false;
#endif
    immersivePortalId_.reset();
    galleryExitSince_={};
    galleryEntryArmed_=false; // do not re-enter until physically leaving the ring
    galleryEntryCandidate_.reset();
    return true;
}

bool XrSwapchainRenderer::saveCurrentScene() {
    if (!sceneRepository_) return false;
    voxen::gallery::SavedRuntime runtime;
    runtime.voxens.reserve(voxens_.size());
    for (const auto& v : voxens_) {
        voxen::gallery::SavedVoxen saved;
        saved.preset=v.preset; saved.position=v.position; saved.velocity=v.velocity;
        saved.scale=v.scale; saved.hue=v.hue; saved.density=v.density;
        saved.speed=v.speed; saved.material=v.material;
        runtime.voxens.push_back(saved);
    }
#ifdef VOXEN_WITH_IMGUI
    const int panoramaIndex=imguiCarousel_.panoramaSelected;
    if(panoramaIndex>=0 && static_cast<std::size_t>(panoramaIndex)<panoramaNames_.size())
        runtime.panoramaFile=panoramaNames_[panoramaIndex];
    runtime.panoramaYawDegrees=imguiCarousel_.panoramaYawDegrees;
    runtime.panoramaHorizontalDegrees=imguiCarousel_.panoramaHorizontalDegrees;
    runtime.panoramaVerticalDegrees=imguiCarousel_.panoramaVerticalDegrees;
    runtime.liquidRoom=imguiCarousel_.liquidRoom;
    runtime.liquidOpacity=imguiCarousel_.liquidOpacity;
    runtime.wallShaderMode=imguiCarousel_.wallShaderMode;
    runtime.cycleWallShaders=imguiCarousel_.cycleWallShaders;
    runtime.wallCycleSeconds=imguiCarousel_.wallCycleSeconds;
    runtime.musicSource=imguiCarousel_.musicSource;
    runtime.musicTrack=imguiCarousel_.musicTrack;
    runtime.musicFile=imguiCarousel_.musicFile;
    runtime.musicVolume=imguiCarousel_.musicVolume;
    runtime.musicPlaying=imguiCarousel_.musicPlaying;
    runtime.motionEnabled=imguiCarousel_.motionEnabled;
    runtime.motionMode=imguiCarousel_.motionMode;
    runtime.motionGravity=imguiCarousel_.motionGravity;
    runtime.motionBounce=imguiCarousel_.motionBounce;
#endif
    const auto payload=voxen::gallery::encodeRuntime(runtime);
    if(payload.empty()) return false;
    auto record=voxen::gallery::SceneRepository::newScene(
        "Scene " + std::to_string(sceneIds_.size()+1));
    record.preview.materialPreset=voxens_.empty()?0u:
        static_cast<std::uint32_t>(voxens_.front().preset);
    // Lightweight proxy: bounding sphere and dominant preset, never a second
    // copy of the Vulkan mesh or full scene in gallery mode.
    if(!voxens_.empty()) {
        std::array<float,3> minP=voxens_.front().position;
        std::array<float,3> maxP=minP;
        for(const auto& v:voxens_) for(int k=0;k<3;++k) {
            minP[k]=std::min(minP[k],v.position[k]-v.scale);
            maxP[k]=std::max(maxP[k],v.position[k]+v.scale);
        }
        float radius2=0.f;
        for(int k=0;k<3;++k) {
            const float half=(maxP[k]-minP[k])*0.5f;
            radius2+=half*half;
        }
        record.preview.boundingRadiusMeters=std::clamp(std::sqrt(radius2),0.05f,99.f);
    }
    if(!sceneRepository_->saveScene(record,payload)) return false;
    refreshSceneIndex();
    const auto it=std::find(sceneIds_.begin(),sceneIds_.end(),record.id);
    selectedSceneIndex_=it==sceneIds_.end()?0:static_cast<int>(it-sceneIds_.begin());
#ifdef VOXEN_WITH_IMGUI
    imguiCarousel_.selectedScene=selectedSceneIndex_;
#endif
    return true;
}

bool XrSwapchainRenderer::deleteSavedScene(const voxen::gallery::SceneId& id) {
    if(!sceneRepository_ || immersiveSceneActive_ ||
       !sceneRepository_->deleteScene(id))return false;
    refreshSceneIndex();
    if(portalManager_)portalManager_->reloadGallery();
    selectedPortalId_.reset();
    selectedSceneIndex_=sceneIds_.empty()?0:
        std::min(selectedSceneIndex_,static_cast<int>(sceneIds_.size()-1));
#ifdef VOXEN_WITH_IMGUI
    imguiCarousel_.selectedScene=selectedSceneIndex_;
#endif
    return true;
}

bool XrSwapchainRenderer::loadSavedScene(const voxen::gallery::SceneId& id) {
    if(!sceneRepository_) return false;
    auto stored=sceneRepository_->loadScene(id);
    if(!stored) return false;
    auto snapshot=voxen::gallery::decodeRuntime(stored->second);
    if(!snapshot) return false;
    std::vector<PlasmaVoxen> restored;
    restored.reserve(snapshot->voxens.size());
    for(const auto& v:snapshot->voxens) {
        PlasmaVoxen item;
        item.preset=v.preset; item.position=v.position; item.velocity=v.velocity;
        item.scale=v.scale; item.hue=v.hue; item.density=v.density;
        item.speed=v.speed; item.material=v.material;
        restored.push_back(item);
    }
    voxens_=std::move(restored);
    lastVoxenValid_=false;
    physicsAccumulator_=0.f;
#ifdef VOXEN_WITH_IMGUI
    rescanPanoramaAssets();
    const auto match=std::find(panoramaNames_.begin(),panoramaNames_.end(),snapshot->panoramaFile);
    imguiCarousel_.panoramaSelected=snapshot->panoramaFile.empty() ||
        match==panoramaNames_.end() ? -1 : static_cast<int>(match-panoramaNames_.begin());
    panoramaDecodedSelection_=-2; // force validation of restored selection next frame
    imguiCarousel_.panoramaYawDegrees=snapshot->panoramaYawDegrees;
    imguiCarousel_.panoramaHorizontalDegrees=snapshot->panoramaHorizontalDegrees;
    imguiCarousel_.panoramaVerticalDegrees=snapshot->panoramaVerticalDegrees;
    if(!snapshot->panoramaFile.empty() && match==panoramaNames_.end())
        imguiCarousel_.panoramaStatus="Saved panorama missing: "+snapshot->panoramaFile;
    imguiCarousel_.liquidRoom=snapshot->liquidRoom;
    imguiCarousel_.liquidOpacity=snapshot->liquidOpacity;
    imguiCarousel_.wallShaderMode=snapshot->wallShaderMode;
    imguiCarousel_.cycleWallShaders=snapshot->cycleWallShaders;
    imguiCarousel_.wallCycleSeconds=snapshot->wallCycleSeconds;
    imguiCarousel_.musicSource=snapshot->musicSource;
    imguiCarousel_.musicTrack=snapshot->musicTrack;
    imguiCarousel_.musicFile=snapshot->musicFile;
    imguiCarousel_.musicVolume=snapshot->musicVolume;
    // Restore the scene's saved music choice, including autoplay on load.
    imguiCarousel_.musicPlaying=snapshot->musicPlaying;
    imguiCarousel_.motionEnabled=snapshot->motionEnabled;
    imguiCarousel_.motionMode=snapshot->motionMode;
    imguiCarousel_.motionGravity=snapshot->motionGravity;
    imguiCarousel_.motionBounce=snapshot->motionBounce;
#endif
    return true;
}

XrSwapchainRenderer::~XrSwapchainRenderer() {
    shutdown();
}

void XrSwapchainRenderer::initialize(
    XrInstance instance,
    XrSystemId systemId,
    VkInstance vulkanInstance,
    ::XrSession session,
    VkDevice device,
    VkPhysicalDevice physicalDevice,
    VkQueue queue,
    uint32_t queueFamilyIndex) {
    session_ = session;
    device_ = device;
    physicalDevice_ = physicalDevice;
    queue_ = queue;

    for (VkFormat candidate : {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT}) {
        VkFormatProperties properties{};
        vkGetPhysicalDeviceFormatProperties(physicalDevice_, candidate, &properties);
        if (properties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
            depthFormat_ = candidate;
            break;
        }
    }
    if (depthFormat_ == VK_FORMAT_UNDEFINED) throw std::runtime_error("No Vulkan depth attachment format supported");
    queueFamilyIndex_ = queueFamilyIndex;

    VkPhysicalDeviceProperties gpuProps{};
    vkGetPhysicalDeviceProperties(physicalDevice_, &gpuProps);
    uint32_t familyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice_, &familyCount, nullptr);
    std::vector<VkQueueFamilyProperties> families(familyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice_, &familyCount, families.data());
    if (queueFamilyIndex_ < familyCount &&
        families[queueFamilyIndex_].timestampValidBits != 0 &&
        gpuProps.limits.timestampComputeAndGraphics == VK_TRUE) {
        VkQueryPoolCreateInfo qi{VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO};
        qi.queryType = VK_QUERY_TYPE_TIMESTAMP;
        qi.queryCount = 2;
        if (vkCreateQueryPool(device_, &qi, nullptr, &gpuTimestampPool_) == VK_SUCCESS)
            timestampPeriodNs_ = gpuProps.limits.timestampPeriod;
    }
    ambientMusic_.start();
    createCommandResources();
    createSwapchains(instance, systemId, session);
#ifdef VOXEN_WITH_IMGUI
    // CPU-side ImGui lifecycle only. GPU texture compositing is a separate step.
    // Do not claim the temporary GLSL panel has been replaced yet.
    if (ImGui::GetCurrentContext() == nullptr) {
        ImGui::CreateContext();
        imguiContextOwned_ = true;
    }
    ImGui::StyleColorsDark();
    auto& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(512.0f, 672.0f);
    io.DeltaTime = 1.f / 72.f;
    io.IniFilename = nullptr;
    // Fetch the rasterized font atlas before the backend uploads it.
    io.Fonts->AddFontDefault();
    io.Fonts->Build();
    imguiSurface_.initialize(vulkanInstance,physicalDevice_,device_,queue_,queueFamilyIndex_);
#endif
    createPipeline();
    // Three shared meshes: high/medium/low detail, extracted only once.
    {
        auto upload=[&](const void* data,VkDeviceSize bytes,VkBufferUsageFlags usage,
                        VkBuffer& buffer,VkDeviceMemory& memory){
            VkBufferCreateInfo bi{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
            bi.size=bytes;bi.usage=usage;bi.sharingMode=VK_SHARING_MODE_EXCLUSIVE;
            checkVk(vkCreateBuffer(device_,&bi,nullptr,&buffer),"vkCreateBuffer(mandelbulb)");
            VkMemoryRequirements req{};
            vkGetBufferMemoryRequirements(device_,buffer,&req);
            VkPhysicalDeviceMemoryProperties props{};
            vkGetPhysicalDeviceMemoryProperties(physicalDevice_,&props);
            uint32_t type=UINT32_MAX;
            for(uint32_t i=0;i<props.memoryTypeCount;i++){
                const auto flags=props.memoryTypes[i].propertyFlags;
                if((req.memoryTypeBits&(1u<<i)) &&
                   (flags&(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))==
                    (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)){
                    type=i;break;
                }
            }
            if(type==UINT32_MAX)throw std::runtime_error("No host-coherent Mandelbulb memory");
            VkMemoryAllocateInfo ai{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
            ai.allocationSize=req.size;ai.memoryTypeIndex=type;
            checkVk(vkAllocateMemory(device_,&ai,nullptr,&memory),"vkAllocateMemory(mandelbulb)");
            checkVk(vkBindBufferMemory(device_,buffer,memory,0),"vkBindBufferMemory(mandelbulb)");
            void* dst=nullptr;
            checkVk(vkMapMemory(device_,memory,0,bytes,0,&dst),"vkMapMemory(mandelbulb)");
            std::memcpy(dst,data,static_cast<size_t>(bytes));
            vkUnmapMemory(device_,memory);
        };
        constexpr int resolutions[3]={44,28,16};
        for(int level=0;level<3;++level){
            const auto mesh=generateIndexedMandelbulbMesh(resolutions[level]);
            if(mesh.vertices.empty() || mesh.indices.empty())continue;
            const auto litVertices=buildMandelbulbLitVertices(mesh);
            auto& lod=mandelbulbLods_[level];
            const VkDeviceSize vb=litVertices.size()*sizeof(MandelbulbLitVertex);
            const VkDeviceSize ib=mesh.indices.size()*sizeof(uint32_t);
            upload(litVertices.data(),vb,VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,lod.vertexBuffer,lod.vertexMemory);
            upload(mesh.indices.data(),ib,VK_BUFFER_USAGE_INDEX_BUFFER_BIT,lod.indexBuffer,lod.indexMemory);
            lod.vertexCount=static_cast<uint32_t>(mesh.vertices.size());
            lod.indexCount=static_cast<uint32_t>(mesh.indices.size());
#ifdef __ANDROID__
            __android_log_print(ANDROID_LOG_INFO,"Voxen",
                "Mandelbulb LOD %d: grid=%d triangles=%u vertices=%u memory=%.2f MiB",
                level,resolutions[level],lod.indexCount/3,lod.vertexCount,
                double(vb+ib)/(1024.0*1024.0));
#endif
        }
    }
    createFramebuffers();
#ifdef VOXEN_WITH_GLB_IMPORT
    if(!pendingDemoGlb_.empty()) {
        loadDemoGlb(pendingDemoGlb_);
        pendingDemoGlb_.clear();
    }
#endif
}

void XrSwapchainRenderer::createCommandResources() {
    VkCommandPoolCreateInfo poolInfo{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamilyIndex_;
    checkVk(vkCreateCommandPool(device_, &poolInfo, nullptr, &commandPool_), "vkCreateCommandPool");

    VkCommandBufferAllocateInfo allocInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    allocInfo.commandPool = commandPool_;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;
    checkVk(vkAllocateCommandBuffers(device_, &allocInfo, &commandBuffer_), "vkAllocateCommandBuffers");

    VkFenceCreateInfo fenceInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    checkVk(vkCreateFence(device_, &fenceInfo, nullptr, &fence_), "vkCreateFence");
}

void XrSwapchainRenderer::createSwapchains(
    XrInstance instance,
    XrSystemId systemId,
    ::XrSession session) {
    uint32_t formatCount = 0;
    checkXr(xrEnumerateSwapchainFormats(session, 0, &formatCount, nullptr), "xrEnumerateSwapchainFormats(count)");
    std::vector<int64_t> formats(formatCount);
    checkXr(xrEnumerateSwapchainFormats(session, formatCount, &formatCount, formats.data()), "xrEnumerateSwapchainFormats");

    const std::array<VkFormat, 3> preferred = {
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_FORMAT_B8G8R8A8_SRGB,
        VK_FORMAT_R8G8B8A8_UNORM,
    };
    for (const auto candidate : preferred) {
        if (std::find(formats.begin(), formats.end(), static_cast<int64_t>(candidate)) != formats.end()) {
            colorFormat_ = candidate;
            break;
        }
    }
    if (colorFormat_ == VK_FORMAT_UNDEFINED) {
        throw std::runtime_error("No supported RGBA OpenXR swapchain format");
    }

    uint32_t viewCount = 0;
    checkXr(
        xrEnumerateViewConfigurationViews(
            instance, systemId, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, 0, &viewCount, nullptr),
        "xrEnumerateViewConfigurationViews(count)");
    if (viewCount != 2) {
        throw std::runtime_error("Voxen currently requires exactly two stereo views");
    }

    std::array<XrViewConfigurationView, 2> configs = {
        XrViewConfigurationView{XR_TYPE_VIEW_CONFIGURATION_VIEW},
        XrViewConfigurationView{XR_TYPE_VIEW_CONFIGURATION_VIEW},
    };
    checkXr(
        xrEnumerateViewConfigurationViews(
            instance,
            systemId,
            XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
            static_cast<uint32_t>(configs.size()),
            &viewCount,
            configs.data()),
        "xrEnumerateViewConfigurationViews");

    for (uint32_t eye = 0; eye < eyes_.size(); ++eye) {
        auto& target = eyes_[eye];
        target.width = static_cast<int32_t>(configs[eye].recommendedImageRectWidth);
        target.height = static_cast<int32_t>(configs[eye].recommendedImageRectHeight);

        XrSwapchainCreateInfo info{XR_TYPE_SWAPCHAIN_CREATE_INFO};
        info.usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT | XR_SWAPCHAIN_USAGE_TRANSFER_DST_BIT;
        info.format = static_cast<int64_t>(colorFormat_);
        info.sampleCount = 1;
        info.width = configs[eye].recommendedImageRectWidth;
        info.height = configs[eye].recommendedImageRectHeight;
        info.faceCount = 1;
        info.arraySize = 1;
        info.mipCount = 1;
        checkXr(xrCreateSwapchain(session, &info, &target.handle), "xrCreateSwapchain");

        uint32_t imageCount = 0;
        checkXr(xrEnumerateSwapchainImages(target.handle, 0, &imageCount, nullptr), "xrEnumerateSwapchainImages(count)");
        target.images.assign(imageCount, {XR_TYPE_SWAPCHAIN_IMAGE_VULKAN2_KHR});
        target.layouts.assign(imageCount, VK_IMAGE_LAYOUT_UNDEFINED);
        checkXr(
            xrEnumerateSwapchainImages(
                target.handle,
                imageCount,
                &imageCount,
                reinterpret_cast<XrSwapchainImageBaseHeader*>(target.images.data())),
            "xrEnumerateSwapchainImages");
    }
}

namespace {

struct Mat4 { float m[16]; };

struct ProceduralPushConstants {
    Mat4 mvp;
    float parameters[4]; // elapsed time, speed, spatial scale, reserved
};
static_assert(sizeof(ProceduralPushConstants) == 80);
struct VolumePushConstants {
    Mat4 mvp;
    float parameters[4];
    float eyeLocal[4];
};
static_assert(sizeof(VolumePushConstants)==96);

Mat4 multiply(const Mat4& a, const Mat4& b) {
    Mat4 r{};
    for (int col = 0; col < 4; ++col)
        for (int row = 0; row < 4; ++row)
            for (int k = 0; k < 4; ++k)
                r.m[col * 4 + row] += a.m[k * 4 + row] * b.m[col * 4 + k];
    return r;
}

Mat4 projection(const XrFovf& fov, float nearZ, float farZ) {
    const float l = std::tan(fov.angleLeft);
    const float r = std::tan(fov.angleRight);
    const float d = std::tan(fov.angleDown);
    const float u = std::tan(fov.angleUp);
    Mat4 m{};
    m.m[0] = 2.0F / (r - l);
    m.m[5] = 2.0F / (u - d);
    m.m[8] = (r + l) / (r - l);
    m.m[9] = (u + d) / (u - d);
    m.m[10] = -farZ / (farZ - nearZ);
    m.m[11] = -1.0F;
    m.m[14] = -(farZ * nearZ) / (farZ - nearZ);
    return m;
}

Mat4 viewMatrix(const XrPosef& pose) {
    // xrLocateViews returns the eye pose in reference-space coordinates.
    // The view matrix is the inverse rigid transform: R(conjugate(q)) * T(-p).
    const float x = -pose.orientation.x;
    const float y = -pose.orientation.y;
    const float z = -pose.orientation.z;
    const float w = pose.orientation.w;

    Mat4 rot{};
    rot.m[0]=1-2*y*y-2*z*z; rot.m[1]=2*x*y+2*z*w; rot.m[2]=2*x*z-2*y*w;
    rot.m[4]=2*x*y-2*z*w; rot.m[5]=1-2*x*x-2*z*z; rot.m[6]=2*y*z+2*x*w;
    rot.m[8]=2*x*z+2*y*w; rot.m[9]=2*y*z-2*x*w; rot.m[10]=1-2*x*x-2*y*y;
    rot.m[15]=1.0F;

    Mat4 t{};
    t.m[0]=t.m[5]=t.m[10]=t.m[15]=1.0F;
    t.m[12]=-pose.position.x;
    t.m[13]=-pose.position.y;
    t.m[14]=-pose.position.z;
    return multiply(rot, t);
}

Mat4 modelMatrix(float x = 0.0F, float y = 0.0F, float scale = 0.25F, float z = -1.5F, float yaw = 0.0F) {
    Mat4 m{};
    const float c = std::cos(yaw), si = std::sin(yaw);
    m.m[0]=c*scale; m.m[2]=-si*scale;
    m.m[5]=scale;
    m.m[8]=si*scale; m.m[10]=c*scale;
    m.m[12]=x;
    m.m[13]=y;
    m.m[15]=1.0F;
    m.m[14]=z;
    return m;
}

// Room surfaces arrive from the Scene provider in LOCAL space (meters).
// Transform the quad's local XY coordinates into the oriented tracked plane.
Mat4 surfaceModel(const voxen::scene::EnvironmentSurface& surface) {
    const auto& q=surface.orientation;
    const float x=q[0],y=q[1],z=q[2],w=q[3];
    Mat4 m{};
    m.m[0]=(1.f-2.f*(y*y+z*z))*surface.halfExtents[0];
    m.m[1]=(2.f*(x*y+z*w))*surface.halfExtents[0];
    m.m[2]=(2.f*(x*z-y*w))*surface.halfExtents[0];
    m.m[4]=(2.f*(x*y-z*w))*surface.halfExtents[1];
    m.m[5]=(1.f-2.f*(x*x+z*z))*surface.halfExtents[1];
    m.m[6]=(2.f*(y*z+x*w))*surface.halfExtents[1];
    m.m[8]=2.f*(x*z+y*w);
    m.m[9]=2.f*(y*z-x*w);
    m.m[10]=1.f-2.f*(x*x+y*y);
    m.m[12]=surface.center[0];
    m.m[13]=surface.center[1];
    m.m[14]=surface.center[2];
    m.m[15]=1.f;
    return m;
}

// A slender cube aligned with the controller's local -Z aiming direction.
Mat4 laserMatrix(const XrPosef& pose, float distance) {
    const auto& q = pose.orientation;
    const float dx = -2.0F * (q.x*q.z + q.w*q.y);
    const float dy = -2.0F * (q.y*q.z - q.w*q.x);
    const float dz = -(1.0F - 2.0F * (q.x*q.x + q.y*q.y));
    // Take a perpendicular sideways basis, then a second perpendicular axis.
    float sx = dz, sy = 0.0F, sz = -dx;
    const float sl = std::sqrt(sx*sx + sz*sz);
    if (sl < 0.001F) { sx=1.0F; sy=0.0F; sz=0.0F; }
    else { sx /= sl; sz /= sl; }
    const float ux = dy*sz - dz*sy;
    const float uy = dz*sx - dx*sz;
    const float uz = dx*sy - dy*sx;
    constexpr float width = 0.006F;
    Mat4 m{};
    m.m[0]=sx*width; m.m[1]=sy*width; m.m[2]=sz*width;
    m.m[4]=ux*width; m.m[5]=uy*width; m.m[6]=uz*width;
    m.m[8]=dx*distance; m.m[9]=dy*distance; m.m[10]=dz*distance;
    m.m[12]=pose.position.x+dx*distance*0.5F;
    m.m[13]=pose.position.y+dy*distance*0.5F;
    m.m[14]=pose.position.z+dz*distance*0.5F;
    m.m[15]=1.0F;
    return m;
}

VkShaderModule makeShader(VkDevice device, const std::uint8_t* bytes, std::size_t size) {
    VkShaderModuleCreateInfo info{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    info.codeSize = size;
    info.pCode = reinterpret_cast<const uint32_t*>(bytes);
    VkShaderModule module = VK_NULL_HANDLE;
    checkVk(vkCreateShaderModule(device, &info, nullptr, &module), "vkCreateShaderModule");
    return module;
}

} // namespace

// Mesh edges are uploaded once and the changing anchor pose is applied by MVP.
static Mat4 meshPoseMatrix(const XrPosef& pose) {
    voxen::scene::EnvironmentSurface proxy{};
    proxy.center={pose.position.x,pose.position.y,pose.position.z};
    proxy.orientation={pose.orientation.x,pose.orientation.y,pose.orientation.z,pose.orientation.w};
    proxy.halfExtents={1.f,1.f};
    return surfaceModel(proxy);
}

void XrSwapchainRenderer::setSceneMeshes(const std::vector<MeshGeometry>& meshes) {
    if(meshes.empty())return;
    // The captured mesh is immutable after Meta Scene discovery. Never rebuild
    // its world-space collision grid on a timer: that causes a visible periodic
    // frame-time spike even when no Voxen is being rendered.
    // The renderer's mesh pose is still refreshed every XR frame.
    const auto& current=meshes.front();
    meshWirePose_=current.pose;
    if(lastCollisionMeshVertices_==current.vertices &&
       lastCollisionMeshIndices_==current.indices)return;
    lastCollisionMeshVertices_=current.vertices;
    lastCollisionMeshIndices_=current.indices;
    // Cache world-space triangles for physics; never read transient XR mesh pointers
    // during the simulation. Scene meshes may be absent until capture completes.
    roomCollisionTriangles_.clear();
    roomCollisionGrid_.clear();
    for(const auto& part:meshes){
        if(!part.vertices || !part.indices)continue;
        const auto& q=part.pose.orientation;
        const auto& origin=part.pose.position;
        auto world=[&](const XrVector3f& p)->std::array<float,3>{
            const float tx=2.f*(q.y*p.z-q.z*p.y);
            const float ty=2.f*(q.z*p.x-q.x*p.z);
            const float tz=2.f*(q.x*p.y-q.y*p.x);
            return {p.x+q.w*tx+(q.y*tz-q.z*ty)+origin.x,
                    p.y+q.w*ty+(q.z*tx-q.x*tz)+origin.y,
                    p.z+q.w*tz+(q.x*ty-q.y*tx)+origin.z};
        };
        for(size_t i=0;i+2<part.indices->size();i+=3){
            const auto a=(*part.indices)[i],b=(*part.indices)[i+1],c=(*part.indices)[i+2];
            if(a>=part.vertices->size()||b>=part.vertices->size()||c>=part.vertices->size())continue;
            const auto wa=world((*part.vertices)[a]),wb=world((*part.vertices)[b]),
                wc=world((*part.vertices)[c]);
            const float ux=wb[0]-wa[0],uy=wb[1]-wa[1],uz=wb[2]-wa[2];
            const float vx=wc[0]-wa[0],vy=wc[1]-wa[1],vz=wc[2]-wa[2];
            const float nx=uy*vz-uz*vy,ny=uz*vx-ux*vz,nz=ux*vy-uy*vx;
            const float n2=nx*nx+ny*ny+nz*nz;
            // Keep near-vertical walls and near-horizontal ceiling candidates.
            if(n2<1e-10f || (ny*ny>0.25f*n2 && ny*ny<0.75f*n2))continue;
            roomCollisionTriangles_.push_back({wa,wb,wc});
        }
    }
    // Index each triangle in a 0.5m spatial grid, once per mesh update.
    // Per-voxel collision checks then visit only overlapping cells.
    constexpr float cellSize=0.5f;
    for(uint32_t i=0;i<roomCollisionTriangles_.size();++i){
        const auto& t=roomCollisionTriangles_[i];
        std::array<int,3> lo{},hi{};
        for(int k=0;k<3;++k){
            lo[k]=static_cast<int>(std::floor(std::min({t.a[k],t.b[k],t.c[k]})/cellSize));
            hi[k]=static_cast<int>(std::floor(std::max({t.a[k],t.b[k],t.c[k]})/cellSize));
        }
        // Huge malformed triangles are excluded from the index for safety.
        if((int64_t(hi[0])-lo[0]+1)*(int64_t(hi[1])-lo[1]+1)*
           (int64_t(hi[2])-lo[2]+1)>4096)continue;
        for(int x=lo[0];x<=hi[0];++x)
            for(int y=lo[1];y<=hi[1];++y)
                for(int z=lo[2];z<=hi[2];++z)
                    roomCollisionGrid_[{x,y,z}].push_back(i);
    }
    if(meshes.empty())return;
    const auto& mesh=meshes.front();
    meshWirePose_=mesh.pose;
    if(meshDepthBuffer_!=VK_NULL_HANDLE || !mesh.vertices || !mesh.indices ||
       mesh.vertices->empty() || mesh.indices->empty())return;
    const auto& indices=*mesh.indices;
    const auto& vertices=*mesh.vertices;
    std::vector<XrVector3f> triangles;
    std::vector<XrVector3f> lines;
    triangles.reserve(indices.size());
    lines.reserve(indices.size()*2);
    for(size_t i=0;i+2<indices.size();i+=3){
        uint32_t a=indices[i],b=indices[i+1],c=indices[i+2];
        if(a>=vertices.size()||b>=vertices.size()||c>=vertices.size())continue;
        triangles.push_back(vertices[a]);triangles.push_back(vertices[b]);triangles.push_back(vertices[c]);
        lines.push_back(vertices[a]);lines.push_back(vertices[b]);
        lines.push_back(vertices[b]);lines.push_back(vertices[c]);
        lines.push_back(vertices[c]);lines.push_back(vertices[a]);
    }
    if(triangles.empty())return;
    VkPhysicalDeviceMemoryProperties properties{};
    vkGetPhysicalDeviceMemoryProperties(physicalDevice_,&properties);
    auto upload=[&](const std::vector<XrVector3f>& src,VkBuffer& buffer,VkDeviceMemory& memory) {
        const VkDeviceSize bytes=src.size()*sizeof(XrVector3f);
        VkBufferCreateInfo bi{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        bi.size=bytes;bi.usage=VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;bi.sharingMode=VK_SHARING_MODE_EXCLUSIVE;
        checkVk(vkCreateBuffer(device_,&bi,nullptr,&buffer),"vkCreateBuffer(scene mesh)");
        VkMemoryRequirements requirements{};
        vkGetBufferMemoryRequirements(device_,buffer,&requirements);
        uint32_t memoryType=UINT32_MAX;
        for(uint32_t i=0;i<properties.memoryTypeCount;i++){
            auto flags=properties.memoryTypes[i].propertyFlags;
            if((requirements.memoryTypeBits&(1u<<i)) &&
                (flags&(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))==
                (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)){
                memoryType=i;break;
            }
        }
        if(memoryType==UINT32_MAX)throw std::runtime_error("No coherent scene mesh memory");
        VkMemoryAllocateInfo ai{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
        ai.allocationSize=requirements.size;ai.memoryTypeIndex=memoryType;
        checkVk(vkAllocateMemory(device_,&ai,nullptr,&memory),"vkAllocateMemory(scene mesh)");
        checkVk(vkBindBufferMemory(device_,buffer,memory,0),"vkBindBufferMemory(scene mesh)");
        void* mapped=nullptr;
        checkVk(vkMapMemory(device_,memory,0,bytes,0,&mapped),"vkMapMemory(scene mesh)");
        std::memcpy(mapped,src.data(),static_cast<size_t>(bytes));
        vkUnmapMemory(device_,memory);
    };
    upload(triangles,meshDepthBuffer_,meshDepthMemory_);
    meshDepthVertexCount_=static_cast<uint32_t>(triangles.size());
    upload(lines,meshWireBuffer_,meshWireMemory_);
    meshWireVertexCount_=static_cast<uint32_t>(lines.size());
#ifdef __ANDROID__
    __android_log_print(ANDROID_LOG_INFO,"VoxenScene",
        "Scene depth upload: triangles=%u wire=%u",meshDepthVertexCount_/3,meshWireVertexCount_);
#endif
}

void XrSwapchainRenderer::createPipeline() {
    VkAttachmentDescription color{};
    color.format = colorFormat_;
    color.samples = VK_SAMPLE_COUNT_1_BIT;
    color.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    color.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription depth{};
    depth.format = depthFormat_;
    depth.samples = VK_SAMPLE_COUNT_1_BIT;
    depth.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depth.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depth.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference ref{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    VkAttachmentReference depthRef{1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &ref;
    subpass.pDepthStencilAttachment = &depthRef;

    VkRenderPassCreateInfo rp{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    const VkAttachmentDescription attachments[] = {color, depth};
    rp.attachmentCount = 2; rp.pAttachments = attachments;
    rp.subpassCount = 1; rp.pSubpasses = &subpass;
    checkVk(vkCreateRenderPass(device_, &rp, nullptr, &renderPass_), "vkCreateRenderPass");

    VkPushConstantRange push{};
    push.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    push.size = sizeof(VolumePushConstants);
    VkPipelineLayoutCreateInfo pli{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    pli.pushConstantRangeCount = 1; pli.pPushConstantRanges = &push;
    checkVk(vkCreatePipelineLayout(device_, &pli, nullptr, &pipelineLayout_), "vkCreatePipelineLayout");

#ifdef VOXEN_WITH_IMGUI
    // The offscreen texture descriptor layout is created by ImGuiSurface.
    VkPushConstantRange panelPush{};
    panelPush.stageFlags=VK_SHADER_STAGE_VERTEX_BIT;
    panelPush.size=sizeof(ProceduralPushConstants);
    VkDescriptorSetLayout uiSetLayout=imguiSurface_.descriptorLayout();
    VkPipelineLayoutCreateInfo panelInfo{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    panelInfo.setLayoutCount=1; panelInfo.pSetLayouts=&uiSetLayout;
    panelInfo.pushConstantRangeCount=1;panelInfo.pPushConstantRanges=&panelPush;
    checkVk(vkCreatePipelineLayout(device_,&panelInfo,nullptr,&imguiPanelPipelineLayout_),"vkCreatePipelineLayout(imgui)");
#endif
#ifdef VOXEN_WITH_GLB_IMPORT
    VkDescriptorSetLayoutBinding binding{};
    binding.binding=0;binding.descriptorType=VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    binding.descriptorCount=1;binding.stageFlags=VK_SHADER_STAGE_FRAGMENT_BIT;
    VkDescriptorSetLayoutCreateInfo info{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    info.bindingCount=1;info.pBindings=&binding;
    checkVk(vkCreateDescriptorSetLayout(device_,&info,nullptr,&glbTextureSetLayout_),"glb set layout");
    VkPipelineLayoutCreateInfo texLayout{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    texLayout.setLayoutCount=1;texLayout.pSetLayouts=&glbTextureSetLayout_;
    texLayout.pushConstantRangeCount=1;texLayout.pPushConstantRanges=&push;
    checkVk(vkCreatePipelineLayout(device_,&texLayout,nullptr,&glbTexturePipelineLayout_),"glb pipeline layout");
#endif
    // Panorama uses a sampled image and 48-byte per-eye orientation/FOV parameters.
    // Descriptor allocation and texture upload are separate from pipeline creation.
    {
        VkDescriptorSetLayoutBinding sampler{};
        sampler.binding=0;
        sampler.descriptorType=VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        sampler.descriptorCount=1;
        sampler.stageFlags=VK_SHADER_STAGE_FRAGMENT_BIT;
        VkDescriptorSetLayoutCreateInfo ci{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
        ci.bindingCount=1;
        ci.pBindings=&sampler;
        checkVk(vkCreateDescriptorSetLayout(device_,&ci,nullptr,&panoramaSetLayout_),
                "panorama descriptor layout");
        VkPushConstantRange range{};
        range.stageFlags=VK_SHADER_STAGE_FRAGMENT_BIT;
        range.offset=0;
        range.size=48;
        VkPipelineLayoutCreateInfo layout{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
        layout.setLayoutCount=1;
        layout.pSetLayouts=&panoramaSetLayout_;
        layout.pushConstantRangeCount=1;
        layout.pPushConstantRanges=&range;
        checkVk(vkCreatePipelineLayout(device_,&layout,nullptr,&panoramaPipelineLayout_),
                "panorama pipeline layout");
    }
    // Shared render pass and push constants.
    auto buildPipeline = [&](const std::uint8_t* vb, std::size_t vsz,
                             const std::uint8_t* fb, std::size_t fsz,
                             VkPipeline& out) {
        VkShaderModule vs = makeShader(device_, vb, vsz);
        VkShaderModule fs = makeShader(device_, fb, fsz);
        VkPipelineShaderStageCreateInfo stages[2]{};
        stages[0] = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
        stages[0].stage=VK_SHADER_STAGE_VERTEX_BIT; stages[0].module=vs; stages[0].pName="main";
        stages[1] = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
        stages[1].stage=VK_SHADER_STAGE_FRAGMENT_BIT; stages[1].module=fs; stages[1].pName="main";
        VkPipelineVertexInputStateCreateInfo vi{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
        VkPipelineInputAssemblyStateCreateInfo ia{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
        ia.topology = (&out==&meshWirePipeline_ || &out==&voxenAttractorPipeline_) ? VK_PRIMITIVE_TOPOLOGY_LINE_LIST : VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        VkVertexInputBindingDescription binding{0,sizeof(XrVector3f),VK_VERTEX_INPUT_RATE_VERTEX};
        VkVertexInputAttributeDescription attribute{0,0,VK_FORMAT_R32G32B32_SFLOAT,0};
        VkVertexInputAttributeDescription litAttributes[2]{
            {0,0,VK_FORMAT_R32G32B32_SFLOAT,0},
            {1,0,VK_FORMAT_R32G32B32_SFLOAT,sizeof(XrVector3f)}
        };
        if(&out==&meshWirePipeline_ || &out==&meshDepthPipeline_ || &out==&voxenMandelbulbPipeline_ || &out==&voxenMandelbulbLitPipeline_
#ifdef VOXEN_WITH_GLB_IMPORT
           || &out==&glbPipeline_ || &out==&glbOriginalPipeline_ || &out==&glbTexturedPipeline_
#endif
        ) {
            binding.stride=sizeof(MandelbulbLitVertex);
            vi.vertexBindingDescriptionCount=1;
            vi.pVertexBindingDescriptions=&binding;
            vi.vertexAttributeDescriptionCount=(&out==&voxenMandelbulbLitPipeline_
#ifdef VOXEN_WITH_GLB_IMPORT
            || &out==&glbPipeline_ || &out==&glbOriginalPipeline_ || &out==&glbTexturedPipeline_
#endif
            ) ? 2 : 1;
            vi.pVertexAttributeDescriptions=(&out==&voxenMandelbulbLitPipeline_
#ifdef VOXEN_WITH_GLB_IMPORT
            || &out==&glbPipeline_ || &out==&glbOriginalPipeline_ || &out==&glbTexturedPipeline_
#endif
            ) ? litAttributes : &attribute;
            if(&out==&meshWirePipeline_ || &out==&meshDepthPipeline_)binding.stride=sizeof(XrVector3f);
        }
#ifdef VOXEN_WITH_GLB_IMPORT
        VkVertexInputAttributeDescription texAttrs[3]{
            {0,0,VK_FORMAT_R32G32B32_SFLOAT,0},{1,0,VK_FORMAT_R32G32B32_SFLOAT,12},
            {2,0,VK_FORMAT_R32G32_SFLOAT,24}};
        if(&out==&glbPipeline_ || &out==&glbOriginalPipeline_ ||
           &out==&glbTexturedPipeline_)binding.stride=32;
        if(&out==&glbTexturedPipeline_) {
            vi.vertexAttributeDescriptionCount=3;
            vi.pVertexAttributeDescriptions=texAttrs;
        }
#endif
        VkPipelineViewportStateCreateInfo vp{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
        vp.viewportCount=1; vp.scissorCount=1;
        VkPipelineRasterizationStateCreateInfo rs{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
        rs.polygonMode=VK_POLYGON_MODE_FILL; rs.cullMode=VK_CULL_MODE_NONE;
        rs.frontFace=VK_FRONT_FACE_COUNTER_CLOCKWISE; rs.lineWidth=1.0F;
        VkPipelineMultisampleStateCreateInfo ms{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
        ms.rasterizationSamples=VK_SAMPLE_COUNT_1_BIT;
        VkPipelineDepthStencilStateCreateInfo ds{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
        ds.depthTestEnable = VK_TRUE;
        ds.depthWriteEnable = (&out == &laserPipeline_ || &out == &liquidRoomPipeline_ || &out == &solidWallPipeline_ || &out == &voxenPlasmaPipeline_ || &out == &voxenSpheresPipeline_ || &out == &voxenMandelbulbPipeline_ || &out == &organicSpherePipeline_ || &out == &torusKnotPipeline_ || &out == &spikeBloomPipeline_ || &out == &jellyfishPipeline_ || &out == &earthwormPipeline_ || &out == &cloudPipeline_ || &out == &metaballsPipeline_ || &out == &voxenAttractorPipeline_ || &out == &meshWirePipeline_ || &out == &carouselPipeline_ || &out == &galleryPortalPipeline_ || &out == &organicSpherePipeline_
#ifdef VOXEN_WITH_IMGUI
        || &out == &imguiPanelPipeline_
#endif
        ) ? VK_FALSE : VK_TRUE;
        if (&out == &panoramaPipeline_) ds.depthTestEnable=VK_FALSE;
        if (&out == &carouselPipeline_
#ifdef VOXEN_WITH_IMGUI
            || &out == &imguiPanelPipeline_
#endif
        ) ds.depthTestEnable=VK_FALSE;
        if(&out==&meshDepthPipeline_){
            ds.depthTestEnable=VK_TRUE;
            ds.depthWriteEnable=VK_TRUE;
        }
        // Solid 3D Voxen must occlude their own rear faces and other objects.
        // Leave the explicitly volumetric/plasma, raymarched and overlay
        // pipelines on their legacy blending paths.
        const bool solidVoxen =
            &out==&organicSpherePipeline_ || &out==&torusKnotPipeline_ ||
            &out==&spikeBloomPipeline_ || &out==&jellyfishPipeline_ ||
            &out==&earthwormPipeline_ || &out==&cloudPipeline_ ||
            &out==&metaballsPipeline_ || &out==&voxenMandelbulbLitPipeline_ ||
            &out==&mechanicalCannonPipeline_
#ifdef VOXEN_WITH_GLB_IMPORT
            || &out==&glbPipeline_ || &out==&glbOriginalPipeline_ || &out==&glbTexturedPipeline_
#endif
            ;
        if(solidVoxen) ds.depthWriteEnable=VK_TRUE;
        if(&out==&panoramaPipeline_)ds.depthWriteEnable=VK_FALSE;
        ds.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
        VkPipelineColorBlendAttachmentState blend{}; blend.colorWriteMask=0xF;
        if (&out == &liquidRoomPipeline_ || &out == &solidWallPipeline_ || &out == &matrixRainPipeline_ || &out == &heppGlowPipeline_ || &out == &phiFieldPipeline_ || &out == &soundDemoPipeline_ || &out == &midnightRidePipeline_ || &out == &seascapePipeline_ || &out == &octgramsPipeline_ || &out == &mandelbrotZoomPipeline_ || &out == &voxenPlasmaPipeline_ || &out == &voxenSpheresPipeline_ || &out == &voxenAttractorPipeline_ || &out == &meshWirePipeline_ || &out == &carouselPipeline_ || &out == &galleryPortalPipeline_
#ifdef VOXEN_WITH_IMGUI
            || &out == &imguiPanelPipeline_
#endif
        ) {
            blend.blendEnable=VK_TRUE;
            blend.srcColorBlendFactor=VK_BLEND_FACTOR_SRC_ALPHA;
            blend.dstColorBlendFactor=VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            blend.colorBlendOp=VK_BLEND_OP_ADD;
            blend.srcAlphaBlendFactor=VK_BLEND_FACTOR_ONE;
            blend.dstAlphaBlendFactor=VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            blend.alphaBlendOp=VK_BLEND_OP_ADD;
        }
        if(solidVoxen) blend.blendEnable=VK_FALSE;
        if(&out==&meshDepthPipeline_) {
            blend.colorWriteMask=0;
            blend.blendEnable=VK_FALSE;
        }
        VkPipelineColorBlendStateCreateInfo cb{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
        cb.attachmentCount=1; cb.pAttachments=&blend;
        VkDynamicState dyns[]={VK_DYNAMIC_STATE_VIEWPORT,VK_DYNAMIC_STATE_SCISSOR};
        VkPipelineDynamicStateCreateInfo dyn{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
        dyn.dynamicStateCount=2; dyn.pDynamicStates=dyns;
    
        VkGraphicsPipelineCreateInfo pi{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
        pi.stageCount=2; pi.pStages=stages; pi.pVertexInputState=&vi; pi.pInputAssemblyState=&ia;
        pi.pViewportState=&vp; pi.pRasterizationState=&rs; pi.pMultisampleState=&ms;
        pi.pColorBlendState=&cb; pi.pDepthStencilState=&ds; pi.pDynamicState=&dyn; pi.layout=pipelineLayout_;
#ifdef VOXEN_WITH_IMGUI
        if(&out==&imguiPanelPipeline_)pi.layout=imguiPanelPipelineLayout_;
#endif
#ifdef VOXEN_WITH_GLB_IMPORT
        if(&out==&glbTexturedPipeline_)pi.layout=glbTexturePipelineLayout_;
#endif
        if(&out==&panoramaPipeline_)pi.layout=panoramaPipelineLayout_;
        pi.renderPass=renderPass_;
        checkVk(vkCreateGraphicsPipelines(device_, VK_NULL_HANDLE, 1, &pi, nullptr, &out), "vkCreateGraphicsPipelines");
        vkDestroyShaderModule(device_, fs, nullptr);
        vkDestroyShaderModule(device_, vs, nullptr);
    };
    buildPipeline(panoramaVert,panoramaVertSize,panoramaFrag,panoramaFragSize,panoramaPipeline_);
    buildPipeline(organicSphereVert, organicSphereVertSize, organicSphereFrag, organicSphereFragSize, organicSpherePipeline_);
    buildPipeline(torusKnotVert, torusKnotVertSize, organicSphereFrag, organicSphereFragSize, torusKnotPipeline_);
    buildPipeline(spikeBloomVert, spikeBloomVertSize, organicSphereFrag, organicSphereFragSize, spikeBloomPipeline_);
    buildPipeline(jellyfishVert, jellyfishVertSize, organicSphereFrag, organicSphereFragSize, jellyfishPipeline_);
    buildPipeline(earthwormVert, earthwormVertSize, organicSphereFrag, organicSphereFragSize, earthwormPipeline_);
    buildPipeline(cloudVert, cloudVertSize, cloudFrag, cloudFragSize, cloudPipeline_);
    buildPipeline(metaballsVert, metaballsVertSize, organicSphereFrag, organicSphereFragSize, metaballsPipeline_);
    buildPipeline(fractalVert, fractalVertSize, voxenPlasmaFrag, voxenPlasmaFragSize, voxenPlasmaPipeline_);
    buildPipeline(fractalVert, fractalVertSize, voxenSpheresFrag, voxenSpheresFragSize, voxenSpheresPipeline_);
    buildPipeline(meshWireVert, meshWireVertSize, laserFrag, laserFragSize, voxenMandelbulbPipeline_);
    buildPipeline(mandelbulbLitVert, mandelbulbLitVertSize, mandelbulbLitFrag, mandelbulbLitFragSize, voxenMandelbulbLitPipeline_);
#ifdef VOXEN_WITH_GLB_IMPORT
    buildPipeline(mandelbulbLitVert,mandelbulbLitVertSize,mandelbulbLitFrag,mandelbulbLitFragSize,glbPipeline_);
    buildPipeline(glbOriginalVert,glbOriginalVertSize,glbOriginalFrag,glbOriginalFragSize,glbOriginalPipeline_);
    buildPipeline(glbTexturedVert,glbTexturedVertSize,glbTexturedFrag,glbTexturedFragSize,glbTexturedPipeline_);
#endif
    buildPipeline(voxenAttractorVert, voxenAttractorVertSize, voxenAttractorFrag, voxenAttractorFragSize, voxenAttractorPipeline_);
    buildPipeline(cubeVert, cubeVertSize, laserFrag, laserFragSize, laserPipeline_);
    buildPipeline(fractalVert, fractalVertSize, liquidRoomFrag, liquidRoomFragSize, liquidRoomPipeline_);
    buildPipeline(fractalVert, fractalVertSize, solidWallFrag, solidWallFragSize, solidWallPipeline_);
    buildPipeline(meshWireVert, meshWireVertSize, meshWireFrag, meshWireFragSize, meshWirePipeline_);
    buildPipeline(meshDepthVert, meshDepthVertSize, meshDepthFrag, meshDepthFragSize, meshDepthPipeline_);
    buildPipeline(fractalVert, fractalVertSize, matrixRainFrag, matrixRainFragSize, matrixRainPipeline_);
    buildPipeline(fractalVert, fractalVertSize, heppGlowFrag, heppGlowFragSize, heppGlowPipeline_);
    buildPipeline(fractalVert, fractalVertSize, phiFieldFrag, phiFieldFragSize, phiFieldPipeline_);
    buildPipeline(fractalVert, fractalVertSize, soundDemoFrag, soundDemoFragSize, soundDemoPipeline_);
    buildPipeline(fractalVert, fractalVertSize, midnightRideFrag, midnightRideFragSize, midnightRidePipeline_);
    buildPipeline(fractalVert, fractalVertSize, seascapeFrag, seascapeFragSize, seascapePipeline_);
    buildPipeline(fractalVert, fractalVertSize, octgramsFrag, octgramsFragSize, octgramsPipeline_);
    buildPipeline(fractalVert, fractalVertSize, mandelbrotZoomFrag, mandelbrotZoomFragSize, mandelbrotZoomPipeline_);
    buildPipeline(fractalVert, fractalVertSize, carouselFrag, carouselFragSize, carouselPipeline_);
    buildPipeline(fractalVert, fractalVertSize, galleryPortalFrag, galleryPortalFragSize, galleryPortalPipeline_);
    buildPipeline(cubeVert, cubeVertSize, mechanicalCannonFrag, mechanicalCannonFragSize, mechanicalCannonPipeline_);
#ifdef VOXEN_WITH_IMGUI
    buildPipeline(fractalVert, fractalVertSize, imguiPanelFrag, imguiPanelFragSize, imguiPanelPipeline_);
#endif
}

void XrSwapchainRenderer::createFramebuffers() {
    VkPhysicalDeviceMemoryProperties memoryProperties{};
    vkGetPhysicalDeviceMemoryProperties(physicalDevice_, &memoryProperties);
    for (auto& eye : eyes_) {
        VkImageCreateInfo imageInfo{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.format = depthFormat_;
        imageInfo.extent = {static_cast<uint32_t>(eye.width), static_cast<uint32_t>(eye.height), 1};
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        checkVk(vkCreateImage(device_, &imageInfo, nullptr, &eye.depthImage), "vkCreateImage(depth)");

        VkMemoryRequirements requirements{};
        vkGetImageMemoryRequirements(device_, eye.depthImage, &requirements);
        uint32_t memoryType = UINT32_MAX;
        for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i) {
            if ((requirements.memoryTypeBits & (1u << i)) &&
                (memoryProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) {
                memoryType = i;
                break;
            }
        }
        if (memoryType == UINT32_MAX) throw std::runtime_error("No device-local memory type for depth image");
        VkMemoryAllocateInfo allocation{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
        allocation.allocationSize = requirements.size;
        allocation.memoryTypeIndex = memoryType;
        checkVk(vkAllocateMemory(device_, &allocation, nullptr, &eye.depthMemory), "vkAllocateMemory(depth)");
        checkVk(vkBindImageMemory(device_, eye.depthImage, eye.depthMemory, 0), "vkBindImageMemory(depth)");

        VkImageViewCreateInfo depthViewInfo{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        depthViewInfo.image = eye.depthImage;
        depthViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        depthViewInfo.format = depthFormat_;
        depthViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        depthViewInfo.subresourceRange.levelCount = 1;
        depthViewInfo.subresourceRange.layerCount = 1;
        checkVk(vkCreateImageView(device_, &depthViewInfo, nullptr, &eye.depthView), "vkCreateImageView(depth)");

        eye.views.resize(eye.images.size());
        eye.framebuffers.resize(eye.images.size());
        for (std::size_t i = 0; i < eye.images.size(); ++i) {
            VkImageViewCreateInfo iv{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
            iv.image = eye.images[i].image;
            iv.viewType = VK_IMAGE_VIEW_TYPE_2D;
            iv.format = colorFormat_;
            iv.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            iv.subresourceRange.levelCount = 1;
            iv.subresourceRange.layerCount = 1;
            checkVk(vkCreateImageView(device_, &iv, nullptr, &eye.views[i]), "vkCreateImageView(color)");

            const VkImageView attachments[] = {eye.views[i], eye.depthView};
            VkFramebufferCreateInfo fb{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
            fb.renderPass = renderPass_;
            fb.attachmentCount = 2;
            fb.pAttachments = attachments;
            fb.width = static_cast<uint32_t>(eye.width);
            fb.height = static_cast<uint32_t>(eye.height);
            fb.layers = 1;
            checkVk(vkCreateFramebuffer(device_, &fb, nullptr, &eye.framebuffers[i]), "vkCreateFramebuffer");
        }
    }
}

void XrSwapchainRenderer::updateControllerInteraction(
    const std::array<XrPosef, 2>& poses,
    const std::array<bool, 2>& tracked,
    const std::array<bool, 2>& pressed,
    const std::array<XrVector2f, 2>& thumbsticks) {
    controllerPoses_=poses;
    controllerTracked_=tracked;
#ifdef VOXEN_WITH_IMGUI
    uiTrigger_[0]=pressed[0];
    uiTrigger_[1]=pressed[1];
#endif
#ifdef VOXEN_WITH_IMGUI
    const bool galleryTriggerEdge=pressed[1] && !lastGalleryTriggerPressed_;
    lastGalleryTriggerPressed_=pressed[1];
    if(pendingGalleryFloorAction_==1 && galleryTriggerEdge && tracked[1] && !imguiCarousel_.visible) {
        const auto& pose=poses[1];
        const auto& q=pose.orientation;
        const std::array<float,3> forward{
            -2.f*(q.x*q.z+q.w*q.y),
            -2.f*(q.y*q.z-q.w*q.x),
            -(1.f-2.f*(q.x*q.x+q.y*q.y))};
        voxen::gallery::Pose placed{};
        placed.positionMeters={
            pose.position.x+0.38f*forward[0],
            pose.position.y+0.38f*forward[1],
            pose.position.z+0.38f*forward[2]};
        const bool valid=portalManager_ && portalManager_->localized() &&
            selectedSceneIndex_>=0 &&
            static_cast<std::size_t>(selectedSceneIndex_)<sceneIds_.size();
        const auto id=valid?portalManager_->placeAtPose(sceneIds_[selectedSceneIndex_],placed):
            std::optional<voxen::gallery::InstanceId>{};
        if(id) {
            selectedPortalId_=*id;
            pendingGalleryFloorAction_=0;
            imguiCarousel_.sceneStatus="Static Voxen placed";
        } else imguiCarousel_.sceneStatus="Placement failed: room anchor or scene unavailable";
    }
#endif
#ifdef VOXEN_WITH_IMGUI
    // Gallery right-hand ray: trigger grabs a sphere, moving controller drags it,
    // releasing trigger persists the final pose relative to the Meta room anchor.
    if(!editingMode() && !immersiveSceneActive_ && pendingGalleryFloorAction_==0 &&
       portalManager_ && portalManager_->localized() && tracked[1]
#ifdef VOXEN_WITH_GLB_IMPORT
       && !glbPreview_
#endif
       ) {
        const auto& hand=poses[1];
        const auto& q=hand.orientation;
        const std::array<float,3> forward{
            -2.f*(q.x*q.z+q.w*q.y),
            -2.f*(q.y*q.z-q.w*q.x),
            -(1.f-2.f*(q.x*q.x+q.y*q.y))};
        if(galleryDragging_) {
            galleryDraggedPose_.positionMeters={
                hand.position.x+galleryGrabDistance_*forward[0],
                hand.position.y+galleryGrabDistance_*forward[1],
                hand.position.z+galleryGrabDistance_*forward[2]};
            if(!pressed[1]) {
                const bool saved=selectedPortalId_ &&
                    portalManager_->moveToPose(*selectedPortalId_,galleryDraggedPose_);
                galleryDragging_=false;
                imguiCarousel_.sceneStatus=saved?"Sphere moved and gallery saved":
                                                  "Move failed: gallery unchanged";
            }
        } else {
            galleryHoveredId_.reset();
            float nearest=10.f;
            for(const auto& portal:portalManager_->visible()) {
                const auto& p=portal.localPose.positionMeters;
                const float dx=p[0]-hand.position.x;
                const float dy=p[1]-hand.position.y;
                const float dz=p[2]-hand.position.z;
                const float t=dx*forward[0]+dy*forward[1]+dz*forward[2];
                if(t<0.08f || t>nearest)continue;
                const float miss2=dx*dx+dy*dy+dz*dz-t*t;
                const float radius=0.25f*std::clamp(portal.scale,0.1f,4.f);
                if(miss2<=radius*radius*2.25f) {
                    nearest=t;
                    galleryHoveredId_=portal.id;
                }
            }
            if(galleryTriggerEdge && galleryHoveredId_) {
                selectedPortalId_=galleryHoveredId_;
                galleryGrabDistance_=nearest;
                galleryDraggedPose_.positionMeters={
                    hand.position.x+nearest*forward[0],
                    hand.position.y+nearest*forward[1],
                    hand.position.z+nearest*forward[2]};
                galleryDragging_=true;
                galleryDeleteArmedUntil_={};
                imguiCarousel_.sceneStatus="Moving sphere; release trigger to save, B twice to delete";
            }
        }
    } else if(galleryDragging_ && !pressed[1]) {
        galleryDragging_=false;
    }
#endif
    if(tracked[0]) {
        float axis=thumbsticks[0].x;
        if(std::fabs(axis)<0.3f)nativeMenuStickLatched_=false;
        if(!nativeMenuStickLatched_ && std::fabs(axis)>0.70f){
            nativeMenuPage_=(nativeMenuPage_+(axis>0?1:4))%5;
            nativeMenuStickLatched_=true;
        }
    }
#ifdef VOXEN_WITH_IMGUI
#ifdef VOXEN_WITH_GLB_IMPORT
    if(glbPreview_ && tracked[1]) {
        const auto& pose=poses[1];
        const auto& q=pose.orientation;
        const std::array<float,3> forward{
            -2.f*(q.x*q.z+q.w*q.y),
            -2.f*(q.y*q.z-q.w*q.x),
            -(1.f-2.f*(q.x*q.x+q.y*q.y))};
        glbPlacement_={pose.position.x+0.38f*forward[0],
                       pose.position.y+0.38f*forward[1],
                       pose.position.z+0.38f*forward[2]};
        const float x=thumbsticks[1].x;
        if(std::fabs(x)<0.3f)glbStickLatched_=false;
        if(!glbStickLatched_ && std::fabs(x)>0.72f) {
            glbScale_=std::clamp(glbScale_*(x>0.f?1.2f:1.f/1.2f),0.01f,2.f);
            glbStickLatched_=true;
        }
        if(galleryTriggerEdge && !panelAimed_) {
            glbPreview_=false;
            glbPlaced_=true;
            glbInGallery_=!editingMode() && !immersiveSceneActive_;
#ifdef VOXEN_WITH_IMGUI
            imguiCarousel_.glbStatus="GLB placed; select a file to preview again";
#endif
        }
    } else glbStickLatched_=false;
#endif
    // In editing mode the right controller holds a live preview.
    // Thumbstick Y selects among 9 presets; X changes size, both debounced.
    if(editingMode() && !immersiveSceneActive_) {
        const float x=thumbsticks[1].x;
        const float y=thumbsticks[1].y;
        if(std::fabs(y)<0.30f)editorPresetStickLatched_=false;
        if(!editorPresetStickLatched_ && std::fabs(y)>0.72f) {
            imguiCarousel_.voxenPreset=(imguiCarousel_.voxenPreset+(y>0.f?1:10))%11;
            editorPresetStickLatched_=true;
        }
        if(std::fabs(x)<0.30f)editorScaleStickLatched_=false;
        if(!editorScaleStickLatched_ && std::fabs(x)>0.72f) {
            imguiCarousel_.voxenScale=std::clamp(
                imguiCarousel_.voxenScale*(x>0.f?1.20f:1.f/1.20f),0.06f,0.60f);
            editorScaleStickLatched_=true;
        }
    } else {
        editorPresetStickLatched_=false;
        editorScaleStickLatched_=false;
    }
    // A single trigger click places exactly one instance. Never place when
    // aiming at the carousel, or during gallery/immersive interactions.
    if(tracked[1] && galleryTriggerEdge && pendingGalleryFloorAction_==0 &&
       editingMode() && !immersiveSceneActive_ &&
       imguiCarousel_.voxenPaintEnabled && !panelAimed_
#ifdef VOXEN_WITH_GLB_IMPORT
       && !glbPreview_
#endif
       ) {
        const auto& pose=poses[1];
        const auto& q=pose.orientation;
        const std::array<float,3> dir{
            -2.f*(q.x*q.z+q.w*q.y),
            -2.f*(q.y*q.z-q.w*q.x),
            -(1.f-2.f*(q.x*q.x+q.y*q.y))};
        PlasmaVoxen v;
        v.preset=imguiCarousel_.voxenPreset;
        v.position={pose.position.x+0.25f*dir[0],
                    pose.position.y+0.25f*dir[1],
                    pose.position.z+0.25f*dir[2]};
        v.scale=std::clamp(imguiCarousel_.voxenScale,0.06f,0.60f);
        v.hue=imguiCarousel_.voxenHue;
        v.density=imguiCarousel_.voxenDensity;
        v.material=v.preset>=4 ? std::clamp(imguiCarousel_.voxenMaterial,0,8) :
            std::clamp(imguiCarousel_.mandelbulbMaterial,0,2);
        v.speed=imguiCarousel_.voxenSpeed;
        v.velocity={dir[0]*imguiCarousel_.motionSpeed,
                    dir[1]*imguiCarousel_.motionSpeed,
                    dir[2]*imguiCarousel_.motionSpeed};
        if(voxens_.size()<256) {
            voxens_.push_back(v);
#ifdef __ANDROID__
            __android_log_print(ANDROID_LOG_INFO,"Voxen",
                "Editor placed preset=%d scale=%.2f total=%zu",
                v.preset,v.scale,voxens_.size());
#endif
        }
    }
#endif
}

void XrSwapchainRenderer::renderImage(EyeSwapchain& eye, uint32_t imageIndex, const XrView& xrView) {
    checkVk(vkResetFences(device_, 1, &fence_), "vkResetFences");
    checkVk(vkResetCommandBuffer(commandBuffer_, 0), "vkResetCommandBuffer");
    VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin.flags=VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    checkVk(vkBeginCommandBuffer(commandBuffer_, &begin), "vkBeginCommandBuffer");

    if (gpuTimestampPool_ != VK_NULL_HANDLE) {
        vkCmdResetQueryPool(commandBuffer_, gpuTimestampPool_, 0, 2);
        vkCmdWriteTimestamp(commandBuffer_, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, gpuTimestampPool_, 0);
    }

#ifdef VOXEN_WITH_IMGUI
    if (imguiSurface_.ready() && ImGui::GetDrawData()!=nullptr && &eye==&eyes_[0])
        imguiSurface_.render(commandBuffer_,ImGui::GetDrawData());
#endif
    auto& layout=eye.layouts.at(imageIndex);
    if (layout != VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
        VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        barrier.oldLayout=layout; barrier.newLayout=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        barrier.srcQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED; barrier.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
        barrier.image=eye.images.at(imageIndex).image;
        barrier.subresourceRange.aspectMask=VK_IMAGE_ASPECT_COLOR_BIT; barrier.subresourceRange.levelCount=1; barrier.subresourceRange.layerCount=1;
        barrier.dstAccessMask=VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        vkCmdPipelineBarrier(commandBuffer_, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                             0,0,nullptr,0,nullptr,1,&barrier);
        layout=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }

    VkClearValue clear{};
#ifdef VOXEN_WITH_IMGUI
    if(imguiCarousel_.panoramaSelected>=0) {
        clear.color.float32[0]=1.f;clear.color.float32[1]=1.f;
        clear.color.float32[2]=1.f;clear.color.float32[3]=1.f;
    } else
#endif
    if(mechanicalCannonPreview_) {
        clear.color.float32[0]=0.0F; clear.color.float32[1]=0.0F;
        clear.color.float32[2]=0.0F; clear.color.float32[3]=1.0F;
    } else {
        clear.color.float32[0]=0.08F; clear.color.float32[1]=0.12F;
        clear.color.float32[2]=0.22F; clear.color.float32[3]=0.0F;
    }
    VkRenderPassBeginInfo rbi{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    rbi.renderPass=renderPass_; rbi.framebuffer=eye.framebuffers.at(imageIndex);
    rbi.renderArea.extent={static_cast<uint32_t>(eye.width),static_cast<uint32_t>(eye.height)};
    VkClearValue clears[2]{};
    clears[0] = clear;
    clears[1].depthStencil = {1.0F, 0};
    rbi.clearValueCount=2; rbi.pClearValues=clears;
    vkCmdBeginRenderPass(commandBuffer_, &rbi, VK_SUBPASS_CONTENTS_INLINE);

    // OpenXR projection uses Y-up; Vulkan's positive-height viewport flips it.
    // Use a negative viewport height to retain the OpenXR world-space orientation.
    VkViewport viewport{0,static_cast<float>(eye.height),static_cast<float>(eye.width),-static_cast<float>(eye.height),0,1};
    VkRect2D scissor{{0,0},{static_cast<uint32_t>(eye.width),static_cast<uint32_t>(eye.height)}};
    vkCmdSetViewport(commandBuffer_,0,1,&viewport); vkCmdSetScissor(commandBuffer_,0,1,&scissor);

#if defined(VOXEN_WITH_GLB_IMPORT) && defined(VOXEN_WITH_IMGUI)
    // Opaque background first, with no depth writes. Real scene objects draw over it.
    if(imguiCarousel_.panoramaSelected>=0 && panoramaTexture_.descriptor && panoramaPipeline_) {
        struct PanoramaPush {
            float orientation[4];
            float tangents[4];
            float controls[4];
        } pc{};
        const auto& q=xrView.pose.orientation;
        pc.orientation[0]=q.x;pc.orientation[1]=q.y;
        pc.orientation[2]=q.z;pc.orientation[3]=q.w;
        pc.tangents[0]=std::tan(xrView.fov.angleLeft);
        pc.tangents[1]=std::tan(xrView.fov.angleRight);
        pc.tangents[2]=std::tan(xrView.fov.angleDown);
        pc.tangents[3]=std::tan(xrView.fov.angleUp);
        constexpr float radians=3.14159265358979323846f/180.f;
        pc.controls[0]=imguiCarousel_.panoramaYawDegrees*radians;
        pc.controls[1]=imguiCarousel_.panoramaHorizontalDegrees*radians;
        pc.controls[2]=imguiCarousel_.panoramaVerticalDegrees*radians;
        pc.controls[3]=panoramaFadeActive_
            ? std::clamp(std::chrono::duration<float>(
                std::chrono::steady_clock::now()-panoramaFadeStart_).count()/1.5f,0.f,1.f)
            : 1.f;
        vkCmdBindPipeline(commandBuffer_,VK_PIPELINE_BIND_POINT_GRAPHICS,panoramaPipeline_);
        vkCmdBindDescriptorSets(commandBuffer_,VK_PIPELINE_BIND_POINT_GRAPHICS,
            panoramaPipelineLayout_,0,1,&panoramaTexture_.descriptor,0,nullptr);
        vkCmdPushConstants(commandBuffer_,panoramaPipelineLayout_,
            VK_SHADER_STAGE_FRAGMENT_BIT,0,sizeof(pc),&pc);
        vkCmdDraw(commandBuffer_,3,1,0,0);
    }
#endif

    const float seconds = std::chrono::duration<float>(
        std::chrono::steady_clock::now() - animationStart_).count();
    const Mat4 projView = multiply(projection(xrView.fov, 0.05F, 100.0F), viewMatrix(xrView.pose));
    // First visible Liquid Room demonstration: a synthetic, labelled quad,
    // NOT a detected wall. Spatial-surface binding comes in the next stage.
    bool showSceneMesh=false;
    bool sceneOcclusion=false;
    float sceneDepthBias=0.002f;
    bool liquidEnabled=true;
    float liquidOpacity=0.42f;
    int maxLiquidWalls=3;
    bool includeFloorCeiling=true;
    int wallShaderMode=0;
    bool cycleWallShaders=true;
    float wallCycleSeconds=30.f;
    std::array<float,3> wallColor{0.12f,0.55f,1.0f};
    float solidOpacity=0.85f;
    float globalSpeed=1.0f;
    bool artworksAnimated=true;
    int motionMode=0;
    float motionAmplitude=0.25f;
#ifdef VOXEN_WITH_IMGUI
    showSceneMesh=imguiCarousel_.showSceneMesh;
    sceneOcclusion=imguiCarousel_.sceneOcclusion;
    sceneDepthBias=std::clamp(imguiCarousel_.sceneDepthBias,0.f,0.01f);
    liquidEnabled=imguiCarousel_.liquidRoom && (editingMode() || immersiveSceneActive_);
    liquidOpacity=std::clamp(imguiCarousel_.liquidOpacity,0.0f,0.6f);
    maxLiquidWalls=std::clamp(imguiCarousel_.maxLiquidWalls,1,8);
    includeFloorCeiling=imguiCarousel_.includeFloorCeiling;
    wallShaderMode=imguiCarousel_.wallShaderMode;
    cycleWallShaders=imguiCarousel_.cycleWallShaders;
    wallCycleSeconds=std::clamp(imguiCarousel_.wallCycleSeconds,10.f,90.f);
    wallColor={imguiCarousel_.wallColor[0],imguiCarousel_.wallColor[1],imguiCarousel_.wallColor[2]};
    solidOpacity=imguiCarousel_.wallSolidOpacity;
    globalSpeed=imguiCarousel_.animationSpeed;
    artworksAnimated=imguiCarousel_.animateArtworks;
    motionMode=imguiCarousel_.artworkMotion;
    motionAmplitude=std::clamp(imguiCarousel_.motionAmplitude,0.0f,0.5f);
#endif
#ifdef VOXEN_WITH_GLB_IMPORT
    // glTF node world matrices are column-major, matching Vulkan push constants.
    // The prototype intentionally uses the existing lighting shader (not PBR).
    if((glbPlaced_ || glbPreview_) && glbPipeline_!=VK_NULL_HANDLE && (glbPreview_ || (glbInGallery_ ? (!editingMode() && !immersiveSceneActive_) : (editingMode() || immersiveSceneActive_)))) {
        bool original=glbOriginalPipeline_!=VK_NULL_HANDLE;
#ifdef VOXEN_WITH_IMGUI
        original=original && imguiCarousel_.glbMaterialMode==0;
#endif

        for(const auto& part:glbPrimitives_) {
            if(part.indexCount==0)continue;
            Mat4 node{};
            std::copy(part.worldMatrix.begin(),part.worldMatrix.end(),node.m);
            const auto root=modelMatrix(glbPlacement_[0],glbPlacement_[1],glbScale_,glbPlacement_[2]);
            const bool textured=original && glbTexturedPipeline_!=VK_NULL_HANDLE &&
                part.textureIndex>=0 && static_cast<size_t>(part.textureIndex)<glbTextures_.size() &&
                glbTextures_[part.textureIndex].descriptor!=VK_NULL_HANDLE;
            const auto active=original?(textured?glbTexturedPipeline_:glbOriginalPipeline_):glbPipeline_;
            const auto layout=textured?glbTexturePipelineLayout_:pipelineLayout_;
            vkCmdBindPipeline(commandBuffer_,VK_PIPELINE_BIND_POINT_GRAPHICS,active);
            if(textured) {
                const auto set=glbTextures_[part.textureIndex].descriptor;
                vkCmdBindDescriptorSets(commandBuffer_,VK_PIPELINE_BIND_POINT_GRAPHICS,layout,0,1,&set,0,nullptr);
            }
            ProceduralPushConstants pc{multiply(projView,multiply(root,node)),{0.f,0.f,0.52f,1.f}};
            if(original)std::copy(part.baseColor.begin(),part.baseColor.end(),pc.parameters);
            vkCmdPushConstants(commandBuffer_,layout,VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT,0,sizeof(pc),&pc);
            const VkDeviceSize off=0;
            vkCmdBindVertexBuffers(commandBuffer_,0,1,&part.vertexBuffer,&off);
            vkCmdBindIndexBuffer(commandBuffer_,part.indexBuffer,0,VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(commandBuffer_,part.indexCount,1,0,0,0);
        }
    }
#endif
    // Depth-only static real-world mesh: occludes wall fragments behind
    // furniture, but never writes opaque pixels over the passthrough layer.
    if(sceneOcclusion && meshDepthVertexCount_>0 && meshDepthBuffer_!=VK_NULL_HANDLE) {
        vkCmdBindPipeline(commandBuffer_,VK_PIPELINE_BIND_POINT_GRAPHICS,meshDepthPipeline_);
        const VkDeviceSize offset=0;
        vkCmdBindVertexBuffers(commandBuffer_,0,1,&meshDepthBuffer_,&offset);
        const auto mvp=multiply(projView,meshPoseMatrix(meshWirePose_));
        ProceduralPushConstants depthConstants{mvp,{sceneDepthBias,0.f,0.f,0.f}};
        vkCmdPushConstants(commandBuffer_,pipelineLayout_,
            VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT,0,sizeof(depthConstants),&depthConstants);
        vkCmdDraw(commandBuffer_,meshDepthVertexCount_,1,0,0);
    }
    if (liquidEnabled && (wallShaderMode!=0 || liquidOpacity>0.001f) &&
        liquidRoomPipeline_ != VK_NULL_HANDLE && solidWallPipeline_ != VK_NULL_HANDLE && matrixRainPipeline_ != VK_NULL_HANDLE && heppGlowPipeline_ != VK_NULL_HANDLE && phiFieldPipeline_ != VK_NULL_HANDLE && soundDemoPipeline_ != VK_NULL_HANDLE && midnightRidePipeline_ != VK_NULL_HANDLE && seascapePipeline_ != VK_NULL_HANDLE && octgramsPipeline_ != VK_NULL_HANDLE && mandelbrotZoomPipeline_ != VK_NULL_HANDLE) {
        // Stable slots are based on scene anchor IDs, not eye-specific
        // distance ordering. Both eyes therefore show the same effect per wall.
        std::vector<uint64_t> shaderIds;
        for(const auto& candidate:environmentSurfaces_.surfaces()) {
            if(candidate.tracked && candidate.classification!=voxen::scene::SurfaceClass::Unknown)
                shaderIds.push_back(candidate.id);
        }
        std::sort(shaderIds.begin(),shaderIds.end());
        shaderIds.erase(std::unique(shaderIds.begin(),shaderIds.end()),shaderIds.end());
        const int cycleStep=static_cast<int>(std::floor(seconds / wallCycleSeconds));
        constexpr int demoModes[]={0,2,3,4,5,6,7,8,9}; // omit solid diagnostic
        auto choosePipeline=[&](int mode)->VkPipeline {
            switch(mode){
                case 1:return solidWallPipeline_;
                case 2:return matrixRainPipeline_;
                case 3:return heppGlowPipeline_;
                case 4:return phiFieldPipeline_;
                case 5:return mandelbrotZoomPipeline_;
                case 6:return soundDemoPipeline_;
                case 7:return midnightRidePipeline_;
                case 8:return seascapePipeline_;
                case 9:return octgramsPipeline_;
                default:return liquidRoomPipeline_;
            }
        };
        // Only verified/tracked wall planes receive a room projection.
        // The synthetic development quad remains a fallback until the native
        // Scene query provider populates EnvironmentSurfaceRegistry.
        bool wallRendered=false;
        struct WallCandidate { const voxen::scene::EnvironmentSurface* surface; float distanceSquared; };
        std::vector<WallCandidate> candidates;
        const auto& camera=xrView.pose.position;
        std::vector<const voxen::scene::EnvironmentSurface*> horizontalSurfaces;
        for(const auto& surface:environmentSurfaces_.surfaces()) {
            if(!surface.tracked || surface.halfExtents[0]<=0.05f ||
               surface.halfExtents[1]<=0.05f)continue;
            if(surface.classification==voxen::scene::SurfaceClass::Floor ||
               surface.classification==voxen::scene::SurfaceClass::Ceiling) {
                if(includeFloorCeiling)horizontalSurfaces.push_back(&surface);
                continue;
            }
            if(surface.classification!=voxen::scene::SurfaceClass::Wall)continue;
            const float dx=surface.center[0]-camera.x;
            const float dy=surface.center[1]-camera.y;
            const float dz=surface.center[2]-camera.z;
            candidates.push_back({&surface,dx*dx+dy*dy+dz*dz});
        }
        // Limit fragment-shader fill cost. Same stable ordering in both eyes.
        std::sort(candidates.begin(),candidates.end(),[](const auto& a,const auto& b) {
            if(a.distanceSquared!=b.distanceSquared)return a.distanceSquared<b.distanceSquared;
            return a.surface->id<b.surface->id;
        });
        const size_t budget=std::min(candidates.size(),static_cast<size_t>(maxLiquidWalls));
        // Floor and ceiling are additional planes, not counted against
        // Max walls. Keep each category to one nearest anchor to bound GPU fill.
        std::vector<const voxen::scene::EnvironmentSurface*> selected;
        selected.reserve(budget+2);
        for(size_t i=0;i<budget;i++)selected.push_back(candidates[i].surface);
        for(auto kind:{voxen::scene::SurfaceClass::Floor,voxen::scene::SurfaceClass::Ceiling}) {
            const voxen::scene::EnvironmentSurface* closest=nullptr;
            float best=1e30f;
            for(const auto* surface:horizontalSurfaces) {
                if(surface->classification!=kind)continue;
                const float dx=surface->center[0]-camera.x;
                const float dy=surface->center[1]-camera.y;
                const float dz=surface->center[2]-camera.z;
                const float distance=dx*dx+dy*dy+dz*dz;
                if(distance<best){best=distance;closest=surface;}
            }
            if(closest)selected.push_back(closest);
        }
        for(const auto* surface:selected) {
            int activeMode=wallShaderMode;
            if(cycleWallShaders){
                auto it=std::lower_bound(shaderIds.begin(),shaderIds.end(),surface->id);
                const auto rank=static_cast<int>(it-shaderIds.begin());
                activeMode=demoModes[(rank+cycleStep)%9];
            }
            vkCmdBindPipeline(commandBuffer_,VK_PIPELINE_BIND_POINT_GRAPHICS,choosePipeline(activeMode));
            const Mat4 wallMvp=multiply(projView,surfaceModel(*surface));
            ProceduralPushConstants liquid{wallMvp,{seconds,globalSpeed,1.0F,liquidOpacity}};
            if(activeMode==1) {
                liquid.parameters[0]=wallColor[0];
                liquid.parameters[1]=wallColor[1];
                liquid.parameters[2]=wallColor[2];
                liquid.parameters[3]=solidOpacity;
            } else if(activeMode==2) {
                liquid.parameters[3]=liquidOpacity;
            }
            vkCmdPushConstants(commandBuffer_,pipelineLayout_,
                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                0,sizeof(liquid),&liquid);
            vkCmdDraw(commandBuffer_,6,1,0,0);
            wallRendered=true;
        }
        if(!wallRendered && !cycleWallShaders && wallShaderMode==0) {
            vkCmdBindPipeline(commandBuffer_,VK_PIPELINE_BIND_POINT_GRAPHICS,liquidRoomPipeline_);
            const Mat4 planeMvp=multiply(projView,
                modelMatrix(0.0F,-0.35F,0.65F,-2.25F,0.0F));
            ProceduralPushConstants liquid{planeMvp,{seconds,globalSpeed,1.0F,liquidOpacity}};
            vkCmdPushConstants(commandBuffer_,pipelineLayout_,
                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                0,sizeof(liquid),&liquid);
            vkCmdDraw(commandBuffer_,6,1,0,0);
        }
    }
#ifdef __ANDROID__
    static bool wireframeStateLogged=false;
    if(showSceneMesh && !wireframeStateLogged) {
        __android_log_print(ANDROID_LOG_INFO,"VoxenScene",
            "Wireframe draw requested: vertices=%u pipeline=%d buffer=%d",
            meshWireVertexCount_,meshWirePipeline_!=VK_NULL_HANDLE,
            meshWireBuffer_!=VK_NULL_HANDLE);
        wireframeStateLogged=true;
    }
#endif
    if(showSceneMesh && meshWireVertexCount_>0 && meshWireBuffer_!=VK_NULL_HANDLE) {
        vkCmdBindPipeline(commandBuffer_,VK_PIPELINE_BIND_POINT_GRAPHICS,meshWirePipeline_);
        const VkDeviceSize offset=0;
        vkCmdBindVertexBuffers(commandBuffer_,0,1,&meshWireBuffer_,&offset);
        const auto mvp=multiply(projView,meshPoseMatrix(meshWirePose_));
        ProceduralPushConstants constants{mvp,{0.f,0.f,0.f,0.f}};
        vkCmdPushConstants(commandBuffer_,pipelineLayout_,
            VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT,0,sizeof(constants),&constants);
        vkCmdDraw(commandBuffer_,meshWireVertexCount_,1,0,0);
    }
    // Opt-in mechanical-cannon stereo preview. Uses the existing procedural cube
    // vertex shader: no per-frame buffer allocations or unverified XR anchors.
    if(mechanicalCannonPreview_ && mechanicalCannonPipeline_!=VK_NULL_HANDLE) {
        const float sceneSeconds=std::chrono::duration<float>(
            std::chrono::steady_clock::now()-mechanicalCannonStart_).count();
        const auto state=voxen::scene::cannon::evaluate(sceneSeconds);
        const auto parts=voxen::scene::cannon::geometry();
        vkCmdBindPipeline(commandBuffer_,VK_PIPELINE_BIND_POINT_GRAPHICS,mechanicalCannonPipeline_);
        for(const auto& part:parts) {
            const float recoil=state.recoil;
            const float x=part.center.x;
            const float y=part.center.y;
            const float z=-10.f+part.center.z-
                ((part.kind==voxen::scene::cannon::Part::Barrel ||
                  part.kind==voxen::scene::cannon::Part::Breech ||
                  part.kind==voxen::scene::cannon::Part::Muzzle)?recoil:0.f);
            Mat4 model=modelMatrix(x,y,1.f,z,0.f);
            // cube.vert covers [-0.5,0.5] in each axis.
            for(int row=0;row<4;++row) {
                model.m[row]*=part.halfExtents.x*2.f;
                model.m[4+row]*=part.halfExtents.y*2.f;
                model.m[8+row]*=part.halfExtents.z*2.f;
            }
            const auto mvp=multiply(projView,model);
            const bool luminous=part.kind==voxen::scene::cannon::Part::ChargingCore ||
                part.kind==voxen::scene::cannon::Part::CoolingRing ||
                part.kind==voxen::scene::cannon::Part::Muzzle;
            const float intensity=luminous?(state.charging+state.muzzleFlash):0.f;
            ProceduralPushConstants pc{mvp,{seconds,intensity,luminous?1.f:0.f,state.exposure}};
            vkCmdPushConstants(commandBuffer_,pipelineLayout_,
                VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT,0,sizeof(pc),&pc);
            vkCmdDraw(commandBuffer_,36,1,0,0);
        }
    if(organicSpherePipeline_!=VK_NULL_HANDLE) {
            vkCmdBindPipeline(commandBuffer_,VK_PIPELINE_BIND_POINT_GRAPHICS,organicSpherePipeline_);
            for(const auto& p:state.projectiles) {
                const auto mvp=multiply(projView,modelMatrix(
                    p.position.x,p.position.y,p.radius,p.position.z,0.f));
                ProceduralPushConstants pc{mvp,{seconds,1.f,4.f+float(p.material),.20f}};
                vkCmdPushConstants(commandBuffer_,pipelineLayout_,
                    VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT,0,sizeof(pc),&pc);
                vkCmdDraw(commandBuffer_,48*32*6,1,0,0);
            }
        }
    }
    // Saved scene instances are static 3D Voxen, not floor rings.
    if(!immersiveSceneActive_ && !editingMode() && portalManager_ &&
       organicSpherePipeline_!=VK_NULL_HANDLE) {
        vkCmdBindPipeline(commandBuffer_,VK_PIPELINE_BIND_POINT_GRAPHICS,
                          organicSpherePipeline_);
        const auto portals=portalManager_->visible();
        const auto count=std::min<std::size_t>(portals.size(),64);
        for(std::size_t i=0;i<count;++i) {
            const auto& p=portals[i];
            const auto& xyz=(galleryDragging_ && selectedPortalId_ && p.id==*selectedPortalId_)
                ? galleryDraggedPose_.positionMeters : p.localPose.positionMeters;
            const float radius=0.125f*std::clamp(p.scale,0.1f,4.f);
            const Mat4 mvp=multiply(projView,
                modelMatrix(xyz[0],xyz[1],radius*2.f,xyz[2],0.f));
            const auto preview=scenePreviewCache_.find(p.sceneId);
            const float preset=preview==scenePreviewCache_.end()?4.f:
                float(4u+preview->second.materialPreset%4u);
            const float selected=(selectedPortalId_ && p.id==*selectedPortalId_) ? 1.5f : 1.f;
            ProceduralPushConstants pc{mvp,{seconds,selected,preset,0.5f}};
            vkCmdPushConstants(commandBuffer_,pipelineLayout_,
                VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT,
                0,sizeof(pc),&pc);
            vkCmdDraw(commandBuffer_,48*32*6,1,0,0);
        }
        // DISPOSE preview follows the right controller until trigger release.
        if(pendingGalleryFloorAction_==1 && controllerTracked_[1]) {
            const auto& pose=controllerPoses_[1];
            const auto& q=pose.orientation;
            const float dx=-2.f*(q.x*q.z+q.w*q.y);
            const float dy=-2.f*(q.y*q.z-q.w*q.x);
            const float dz=-(1.f-2.f*(q.x*q.x+q.y*q.y));
            const Mat4 mvp=multiply(projView,modelMatrix(
                pose.position.x+0.38f*dx,pose.position.y+0.38f*dy,
                0.25f,pose.position.z+0.38f*dz,0.f));
            ProceduralPushConstants pc{mvp,{seconds,1.f,4.f,0.5f}};
            vkCmdPushConstants(commandBuffer_,pipelineLayout_,
                VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT,
                0,sizeof(pc),&pc);
            vkCmdDraw(commandBuffer_,48*32*6,1,0,0);
        }
    }
    // Gallery hands are tracked for pinches and UI interaction, but no
    // procedural fire or fingertip spheres are rendered.
    // A skinned XR_FB_hand_tracking_mesh renderer will be added separately.
    if(voxenPlasmaPipeline_!=VK_NULL_HANDLE && voxenSpheresPipeline_!=VK_NULL_HANDLE &&
       voxenMandelbulbPipeline_!=VK_NULL_HANDLE && voxenAttractorPipeline_!=VK_NULL_HANDLE && organicSpherePipeline_!=VK_NULL_HANDLE && torusKnotPipeline_!=VK_NULL_HANDLE && spikeBloomPipeline_!=VK_NULL_HANDLE && jellyfishPipeline_!=VK_NULL_HANDLE && earthwormPipeline_!=VK_NULL_HANDLE && cloudPipeline_!=VK_NULL_HANDLE && metaballsPipeline_!=VK_NULL_HANDLE){
        auto paintPlasma=[&](const PlasmaVoxen& v){
            vkCmdBindPipeline(commandBuffer_,VK_PIPELINE_BIND_POINT_GRAPHICS,
                v.preset==1 ? voxenSpheresPipeline_ :
                v.preset==2 ? (
#ifdef VOXEN_WITH_IMGUI
                imguiCarousel_.mandelbulbLighting ? voxenMandelbulbLitPipeline_ :
#endif
                voxenMandelbulbPipeline_) :
                v.preset==3 ? voxenAttractorPipeline_ : v.preset==4 ? organicSpherePipeline_ :
                v.preset==5 ? torusKnotPipeline_ : v.preset==6 ? spikeBloomPipeline_ :
                v.preset==7 ? jellyfishPipeline_ : v.preset==8 ? earthwormPipeline_ : v.preset==9 ? cloudPipeline_ : v.preset==10 ? metaballsPipeline_ : voxenPlasmaPipeline_);
            const Mat4 mvp=multiply(projView,modelMatrix(
                v.position[0],v.position[1],v.scale,v.position[2],0.f));
            // For Mandelbulbs, pack material into the otherwise unchanged float slot.
            // Density remains in [0.3,2.0], while material uses multiples of 4.
            const float packedDensity=v.preset==2 ? v.density+4.0f*float(v.material) : v.density;
            ProceduralPushConstants pc{mvp,{seconds,v.speed,v.hue+(v.preset>=4?float(v.material):0.f),v.preset>=4 ? std::clamp(v.density*0.20f,0.06f,0.40f) : packedDensity}};
            vkCmdPushConstants(commandBuffer_,pipelineLayout_,
                VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT,0,sizeof(pc),&pc);
            if(v.preset==2){
                const auto& lod=mandelbulbLods_[std::clamp(v.lodLevel,0,2)];
                if(lod.indexCount>0){
                    VkDeviceSize offset=0;
                    vkCmdBindVertexBuffers(commandBuffer_,0,1,&lod.vertexBuffer,&offset);
                    vkCmdBindIndexBuffer(commandBuffer_,lod.indexBuffer,0,VK_INDEX_TYPE_UINT32);
                    vkCmdDrawIndexed(commandBuffer_,lod.indexCount,1,0,0,0);
                }
            }else{
                vkCmdDraw(commandBuffer_,v.preset==8 ? 12*11*6 : (v.preset==10 ? 32*24*6 : (v.preset==7 ? 24*16*6 : (v.preset>=4 ? 48*32*6 : (v.preset==3 ? 24*18*2 : 6)))),1,0,0);
            }
        };
        for(const auto& v:voxens_)paintPlasma(v);
#ifdef VOXEN_WITH_IMGUI
        // Only one right-hand tool preview can be active at once.
        // Live selected Voxen preview, not part of the saved scene.
        if(editingMode() && imguiCarousel_.voxenPaintEnabled &&
#ifdef VOXEN_WITH_GLB_IMPORT
           !glbPreview_ &&
#endif
           controllerTracked_[1] && !panelAimed_) {
            const auto& pose=controllerPoses_[1];
            const auto& q=pose.orientation;
            const float dx=-2.f*(q.x*q.z+q.w*q.y);
            const float dy=-2.f*(q.y*q.z-q.w*q.x);
            const float dz=-(1.f-2.f*(q.x*q.x+q.y*q.y));
            PlasmaVoxen preview;
            preview.preset=imguiCarousel_.voxenPreset;
            preview.position={pose.position.x+0.25f*dx,
                              pose.position.y+0.25f*dy,
                              pose.position.z+0.25f*dz};
            preview.scale=std::clamp(imguiCarousel_.voxenScale,0.06f,0.60f);
            preview.hue=imguiCarousel_.voxenHue;
            preview.density=imguiCarousel_.voxenDensity;
            preview.speed=imguiCarousel_.voxenSpeed;
            preview.material=preview.preset>=4 ?
                std::clamp(imguiCarousel_.voxenMaterial,0,8) :
                std::clamp(imguiCarousel_.mandelbulbMaterial,0,2);
            preview.lodLevel=1; // Stable medium LOD for Mandelbulb preview.
            paintPlasma(preview);
        }

#endif
    }
#ifdef VOXEN_WITH_IMGUI
    // Minimal native eight-page carousel scaffold, left-hand anchored.
    if (imguiCarousel_.visible && controllerTracked_[0] && imguiSurface_.ready() && imguiPanelPipeline_ != VK_NULL_HANDLE) {
        const auto& cp=controllerPoses_[0].position;
        // Keep the panel at a readable distance from both eyes, rather than
        // having the left controller push it through the headset near plane.
        const auto& eye=xrView.pose.position;
        float px=cp.x, py=cp.y+0.17f, pz=cp.z;
        const float dx=px-eye.x, dz=pz-eye.z;
        const float distance=std::sqrt(dx*dx+dz*dz);
        if (distance<0.55f) {
            // Eye/controller coincidence: use the tracked eye's forward direction.
            const auto& q=xrView.pose.orientation;
            const float fx=-2.0f*(q.x*q.z+q.w*q.y);
            const float fz=-(1.0f-2.0f*(q.x*q.x+q.y*q.y));
            const float inv=distance>0.05f ? 1.0f/distance : 0.0f;
            px=eye.x+(inv>0.0f?dx*inv:fx)*0.70f;
            pz=eye.z+(inv>0.0f?dz*inv:fz)*0.70f;
        }
        // Normal faces the eyes. 22 cm high, with base near the controller.
        const float yaw=std::atan2(eye.x-px,eye.z-pz);
        Mat4 menuModel=modelMatrix(px,py,0.22f,pz,yaw);
        menuModel.m[5]*=(672.f/512.f);
        const Mat4 menuMvp=multiply(projView,menuModel);
        ProceduralPushConstants menu{menuMvp,{seconds,1.0f,1.0f,float(nativeMenuPage_)}};
        vkCmdBindPipeline(commandBuffer_,VK_PIPELINE_BIND_POINT_GRAPHICS,imguiPanelPipeline_);
        VkDescriptorSet textureSet=imguiSurface_.descriptor();
        vkCmdBindDescriptorSets(commandBuffer_,VK_PIPELINE_BIND_POINT_GRAPHICS,
            imguiPanelPipelineLayout_,0,1,&textureSet,0,nullptr);
        vkCmdPushConstants(commandBuffer_,imguiPanelPipelineLayout_,
            VK_SHADER_STAGE_VERTEX_BIT,
            0,sizeof(menu),&menu);
        vkCmdDraw(commandBuffer_,6,1,0,0);
    }
#endif
    // Ray is shown only on the right controller when it points at the UI.
#ifdef VOXEN_WITH_IMGUI
    if(controllerTracked_[1] && panelAimed_){
        vkCmdBindPipeline(commandBuffer_,VK_PIPELINE_BIND_POINT_GRAPHICS,laserPipeline_);
        const float length=std::max(0.05f,panelHitDistance_);
        const Mat4 beamMvp=multiply(projView,laserMatrix(controllerPoses_[1],length));
        ProceduralPushConstants constants{beamMvp,{seconds,1.0F,1.0F,0.0F}};
        vkCmdPushConstants(commandBuffer_,pipelineLayout_,
            VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT,0,sizeof(constants),&constants);
        vkCmdDraw(commandBuffer_,36,1,0,0);
    }
#endif
    vkCmdEndRenderPass(commandBuffer_);
    if (gpuTimestampPool_ != VK_NULL_HANDLE)
        vkCmdWriteTimestamp(commandBuffer_, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, gpuTimestampPool_, 1);
    checkVk(vkEndCommandBuffer(commandBuffer_), "vkEndCommandBuffer");
    VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO}; submit.commandBufferCount=1; submit.pCommandBuffers=&commandBuffer_;
    checkVk(vkQueueSubmit(queue_,1,&submit,fence_), "vkQueueSubmit");
    checkVk(vkWaitForFences(device_,1,&fence_,VK_TRUE,UINT64_MAX), "vkWaitForFences");
    if (gpuTimestampPool_ != VK_NULL_HANDLE) {
        uint64_t ts[2]{};
        if (vkGetQueryPoolResults(device_,gpuTimestampPool_,0,2,sizeof(ts),ts,sizeof(uint64_t),
                VK_QUERY_RESULT_64_BIT) == VK_SUCCESS && ts[1] >= ts[0]) {
            const float ms = static_cast<float>(ts[1]-ts[0])*timestampPeriodNs_*1e-6f;
            lastGpuMs_ = completedGpuSamples_++ == 0 ? ms : lastGpuMs_*0.9f+ms*0.1f;
        }
    }
}

// Fixed-step, world-space sphere proxy physics. One update per stereo frame.
// The procedural surface deformation stays entirely in the Vulkan shaders.
void XrSwapchainRenderer::advanceVoxenPhysics(float dt, bool gravity, float bounce,
    int mode, float swarmRadius, float swarmStrength, float swarmSeparation, bool collideRoomMesh) {
    constexpr float kMaxSpeed=4.f;
    if(mode==1 && voxens_.size()>1){
        // Snapshot neighbor state: steering is independent of iteration order.
        std::vector<std::array<float,3>> acceleration(voxens_.size());
        const float radius2=swarmRadius*swarmRadius;
        for(size_t i=0;i<voxens_.size();++i){
            std::array<float,3> cohesion{}, alignment{}, separation{};
            int count=0;
            for(size_t j=0;j<voxens_.size();++j){
                if(i==j)continue;
                float delta[3];
                for(int k=0;k<3;++k)delta[k]=voxens_[j].position[k]-voxens_[i].position[k];
                const float d2=delta[0]*delta[0]+delta[1]*delta[1]+delta[2]*delta[2];
                if(d2<1e-8f || d2>radius2)continue;
                ++count;
                for(int k=0;k<3;++k){
                    cohesion[k]+=delta[k];
                    alignment[k]+=voxens_[j].velocity[k]-voxens_[i].velocity[k];
                    separation[k]-=delta[k]/std::max(d2,0.015f);
                }
            }
            if(count){
                const float inv=1.f/static_cast<float>(count);
                for(int k=0;k<3;++k){
                    acceleration[i][k]=swarmStrength*(0.85f*cohesion[k]*inv+
                        0.65f*alignment[k]*inv+
                        0.09f*swarmSeparation*separation[k]*inv);
                }
            }
        }
        for(size_t i=0;i<voxens_.size();++i){
            for(int k=0;k<3;++k){
                voxens_[i].velocity[k]+=std::clamp(acceleration[i][k],-5.f,5.f)*dt;
                voxens_[i].velocity[k]*=1.f-0.12f*dt;
            }
        }
    }
    for(auto& v:voxens_){
        if(gravity) v.velocity[1]-=9.81f*dt;
        for(int k=0;k<3;++k){
            v.velocity[k]=std::clamp(v.velocity[k],-kMaxSpeed,kMaxSpeed);
            v.position[k]+=v.velocity[k]*dt;
        }
        const float radius=std::max(0.025f,v.scale*0.5f);
        // Lightweight hand interaction: each tracked palm is one static sphere.
        // Only active with the carousel hidden. No fingers, controller poses,
        // or mesh collision tests; cost is at most two checks per moving voxen.
        if(!carouselVisible()) {
            constexpr float palmRadius=0.105f;
            for(const auto& hand:holographicHands_) {
                if(!hand.tracked)continue;
                const auto& palm=hand.joints[XR_HAND_JOINT_PALM_EXT];
                const float dx=v.position[0]-palm.x;
                const float dy=v.position[1]-palm.y;
                const float dz=v.position[2]-palm.z;
                const float minDistance=radius+palmRadius;
                const float d2=dx*dx+dy*dy+dz*dz;
                if(d2>=minDistance*minDistance)continue;
                const float distance=std::sqrt(d2);
                // If centers coincide, use a stable upward normal.
                const float nx=distance>1e-5f?dx/distance:0.f;
                const float ny=distance>1e-5f?dy/distance:1.f;
                const float nz=distance>1e-5f?dz/distance:0.f;
                v.position[0]=palm.x+nx*minDistance;
                v.position[1]=palm.y+ny*minDistance;
                v.position[2]=palm.z+nz*minDistance;
                // Reflect relative to the moving palm, not a static obstacle.
                // A fast palm can now impart momentum to an otherwise still voxen.
                const float relativeNormalSpeed=
                    (v.velocity[0]-hand.palmVelocity.x)*nx+
                    (v.velocity[1]-hand.palmVelocity.y)*ny+
                    (v.velocity[2]-hand.palmVelocity.z)*nz;
                if(relativeNormalSpeed<0.f) {
                    const float impulse=(1.f+std::clamp(bounce,0.f,1.f))*
                                        relativeNormalSpeed;
                    v.velocity[0]-=impulse*nx;
                    v.velocity[1]-=impulse*ny;
                    v.velocity[2]-=impulse*nz;
                    const float speed=std::sqrt(v.velocity[0]*v.velocity[0]+
                                                v.velocity[1]*v.velocity[1]+
                                                v.velocity[2]*v.velocity[2]);
                    if(speed>4.f) {
                        const float limit=4.f/speed;
                        for(float& component:v.velocity)component*=limit;
                    }
                }
            }
        }
        if(hasTrackedFloor_ && v.position[1]-radius<trackedFloorY_){
            v.position[1]=trackedFloorY_+radius;
            if(v.velocity[1]<0.f) v.velocity[1]=-v.velocity[1]*bounce;
            if(gravity && std::fabs(v.velocity[1])<0.07f) v.velocity[1]=0.f;
        }
    }
    if(collideRoomMesh && !roomCollisionTriangles_.empty()){
        // Sphere against actual scene triangles (closest point on triangle).
        // Broad-phase AABB avoids expensive closest-point tests for distant faces.
        auto sub=[](const std::array<float,3>& a,const std::array<float,3>& b){
            return std::array<float,3>{a[0]-b[0],a[1]-b[1],a[2]-b[2]};};
        auto dot=[](const std::array<float,3>& a,const std::array<float,3>& b){
            return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];};
        auto closest=[&](const std::array<float,3>& p,const CollisionTriangle& t){
            const auto ab=sub(t.b,t.a),ac=sub(t.c,t.a),ap=sub(p,t.a);
            const float d1=dot(ab,ap),d2=dot(ac,ap);
            if(d1<=0.f&&d2<=0.f)return t.a;
            const auto bp=sub(p,t.b);
            const float d3=dot(ab,bp),d4=dot(ac,bp);
            if(d3>=0.f&&d4<=d3)return t.b;
            const float vc=d1*d4-d3*d2;
            if(vc<=0.f&&d1>=0.f&&d3<=0.f){
                const float v=d1/(d1-d3);
                return std::array<float,3>{t.a[0]+v*ab[0],t.a[1]+v*ab[1],t.a[2]+v*ab[2]};}
            const auto cp=sub(p,t.c);
            const float d5=dot(ab,cp),d6=dot(ac,cp);
            if(d6>=0.f&&d5<=d6)return t.c;
            const float vb=d5*d2-d1*d6;
            if(vb<=0.f&&d2>=0.f&&d6<=0.f){
                const float w=d2/(d2-d6);
                return std::array<float,3>{t.a[0]+w*ac[0],t.a[1]+w*ac[1],t.a[2]+w*ac[2]};}
            const float va=d3*d6-d5*d4;
            if(va<=0.f&&(d4-d3)>=0.f&&(d5-d6)>=0.f){
                const auto bc=sub(t.c,t.b);
                const float w=(d4-d3)/((d4-d3)+(d5-d6));
                return std::array<float,3>{t.b[0]+w*bc[0],t.b[1]+w*bc[1],t.b[2]+w*bc[2]};}
            const float denom=va+vb+vc;
            if(std::fabs(denom)<1e-10f)return t.a;
            const float inv=1.f/denom,v=vb*inv,w=vc*inv;
            return std::array<float,3>{t.a[0]+v*ab[0]+w*ac[0],
                t.a[1]+v*ab[1]+w*ac[1],t.a[2]+v*ab[2]+w*ac[2]};
        };
        for(auto& v:voxens_){
            const float radius=std::max(0.025f,v.scale*0.5f);
            std::vector<uint32_t> candidates;
            std::array<int,3> lo{},hi{};
            for(int k=0;k<3;++k){
                lo[k]=static_cast<int>(std::floor((v.position[k]-radius)/0.5f));
                hi[k]=static_cast<int>(std::floor((v.position[k]+radius)/0.5f));
            }
            for(int x=lo[0];x<=hi[0];++x)
                for(int y=lo[1];y<=hi[1];++y)
                    for(int z=lo[2];z<=hi[2];++z){
                        auto it=roomCollisionGrid_.find({x,y,z});
                        if(it!=roomCollisionGrid_.end())
                            candidates.insert(candidates.end(),it->second.begin(),it->second.end());
                    }
            std::sort(candidates.begin(),candidates.end());
            candidates.erase(std::unique(candidates.begin(),candidates.end()),candidates.end());
            for(uint32_t triangleIndex:candidates){
                const auto& t=roomCollisionTriangles_[triangleIndex];
                // Only horizontal faces sufficiently above the calibrated floor are ceilings.
                const float abx=t.b[0]-t.a[0],aby=t.b[1]-t.a[1],abz=t.b[2]-t.a[2];
                const float acx=t.c[0]-t.a[0],acy=t.c[1]-t.a[1],acz=t.c[2]-t.a[2];
                const float nx=aby*acz-abz*acy,ny=abz*acx-abx*acz,nz=abx*acy-aby*acx;
                const float norm2=nx*nx+ny*ny+nz*nz;
                if(ny*ny>=0.75f*norm2 &&
                   (!hasTrackedFloor_ || std::min({t.a[1],t.b[1],t.c[1]})<trackedFloorY_+1.8f))
                    continue;
                bool distant=false;
                for(int k=0;k<3;++k){
                    const float lo=std::min({t.a[k],t.b[k],t.c[k]})-radius;
                    const float hi=std::max({t.a[k],t.b[k],t.c[k]})+radius;
                    if(v.position[k]<lo||v.position[k]>hi){distant=true;break;}
                }
                if(distant)continue;
                const auto p=closest(v.position,t);
                auto delta=sub(v.position,p);
                float d2=dot(delta,delta);
                if(d2>=radius*radius)continue;
                const float d=std::sqrt(d2);
                if(d<1e-6f){
                    const auto ab=sub(t.b,t.a),ac=sub(t.c,t.a);
                    delta={ab[1]*ac[2]-ab[2]*ac[1],
                           ab[2]*ac[0]-ab[0]*ac[2],
                           ab[0]*ac[1]-ab[1]*ac[0]};
                    const float length=std::sqrt(dot(delta,delta));
                    if(length<1e-7f)continue;
                    for(float& x:delta)x/=length;
                    if(dot(delta,v.velocity)>0.f)for(float& x:delta)x=-x;
                }else for(float& x:delta)x/=d;
                for(int k=0;k<3;++k)v.position[k]+=delta[k]*(radius-d+0.001f);
                const float vn=dot(v.velocity,delta);
                if(vn<0.f)for(int k=0;k<3;++k)v.velocity[k]-=(1.f+bounce)*vn*delta[k];
            }
        }
    }
    // At most 256 voxens: O(n^2) narrow phase is fine for this first prototype.
    for(size_t i=0;i<voxens_.size();++i){
        for(size_t j=i+1;j<voxens_.size();++j){
            auto& a=voxens_[i]; auto& b=voxens_[j];
            const float radius=0.5f*(a.scale+b.scale);
            const float dx=b.position[0]-a.position[0];
            const float dy=b.position[1]-a.position[1];
            const float dz=b.position[2]-a.position[2];
            const float dist2=dx*dx+dy*dy+dz*dz;
            if(dist2>=radius*radius) continue;
            const float dist=std::sqrt(dist2);
            const float nx=dist>1e-5f?dx/dist:1.f;
            const float ny=dist>1e-5f?dy/dist:0.f;
            const float nz=dist>1e-5f?dz/dist:0.f;
            const float penetration=radius-dist;
            const float normal[3]={nx,ny,nz};
            for(int k=0;k<3;++k){
                a.position[k]-=normal[k]*penetration*0.5f;
                b.position[k]+=normal[k]*penetration*0.5f;
            }
            const float relative=(b.velocity[0]-a.velocity[0])*nx+
                (b.velocity[1]-a.velocity[1])*ny+(b.velocity[2]-a.velocity[2])*nz;
            if(relative<0.f){
                const float impulse=-(1.f+bounce)*relative*0.5f; // equal masses
                for(int k=0;k<3;++k){
                    a.velocity[k]-=impulse*normal[k];
                    b.velocity[k]+=impulse*normal[k];
                }
            }
        }
    }
}

void XrSwapchainRenderer::render(
    const std::array<XrView, 2>& views,
    std::array<XrCompositionLayerProjectionView, 2>& projectionViews) {
#ifdef VOXEN_WITH_IMGUI
    if (ImGui::GetCurrentContext() != nullptr) {
        const auto now=std::chrono::steady_clock::now();
        const float dt=lastUiFrame_.time_since_epoch().count() == 0
            ? 1.f/72.f
            : std::clamp(std::chrono::duration<float>(now-lastUiFrame_).count(),1.f/240.f,0.1f);
        lastUiFrame_=now;
        auto& io=ImGui::GetIO();
        io.DeltaTime=dt;
        // Project the right-hand aim ray to the same left-controller panel
        // transform used by the stereo pass.
        bool aiming=false;
        float cursorX=0.f,cursorY=0.f;
        if(imguiCarousel_.visible && controllerTracked_[0] && controllerTracked_[1]) {
            const auto& cp=controllerPoses_[0].position;
            const auto& eye=views[0].pose.position;
            float px=cp.x,py=cp.y+0.29f,pz=cp.z;
            const float dx=px-eye.x,dz=pz-eye.z;
            const float distance=std::sqrt(dx*dx+dz*dz);
            if(distance<0.70f) {
                const auto& q=views[0].pose.orientation;
                const float fx=-2.f*(q.x*q.z+q.w*q.y);
                const float fz=-(1.f-2.f*(q.x*q.x+q.y*q.y));
                const float inv=distance>0.05f?1.f/distance:0.f;
                px=eye.x+(inv>0?dx*inv:fx)*0.70f;
                pz=eye.z+(inv>0?dz*inv:fz)*0.70f;
            }
            const float yaw=std::atan2(eye.x-px,eye.z-pz);
            XrPosef panel{};
            panel.position={px,py,pz};
            panel.orientation={0.f,std::sin(yaw*0.5f),0.f,std::cos(yaw*0.5f)};
            auto hit=voxen::ui::rayToPanel(controllerPoses_[1],panel,0.44f,0.44f*672.f/512.f);
            if(hit.valid) {
                aiming=true;
                panelHitDistance_=hit.distance;
                cursorX=hit.u*512.f;cursorY=hit.v*672.f;
                io.AddMousePosEvent(cursorX,cursorY);
            }
        }
        if(!aiming) io.AddMousePosEvent(-FLT_MAX,-FLT_MAX);
        io.AddMouseButtonEvent(0,aiming && uiTrigger_[1]);
        panelAimed_=aiming;
        if (!aiming) panelHitDistance_=0.0f;
        imguiCarousel_.page=nativeMenuPage_;
        imguiCarousel_.gpuMs=lastGpuMs_;
        imguiCarousel_.gpuAvailable=gpuTimingAvailable() && completedGpuSamples_>0;
        ImGui_ImplVulkan_NewFrame();
        ImGui::NewFrame();
        imguiCarousel_.voxenCount=static_cast<int>(voxens_.size());
        imguiCarousel_.musicReady=ambientMusic_.ready();
#ifdef VOXEN_WITH_GLB_IMPORT
        imguiCarousel_.glbFiles=glbAssetNames_;
        if(imguiCarousel_.glbPreviewRequest && !glbAssetPaths_.empty() && imguiCarousel_.glbSelected>=0 &&
           static_cast<size_t>(imguiCarousel_.glbSelected)<glbAssetPaths_.size()) {
            imguiCarousel_.glbPreviewRequest=false;
            if(loadDemoGlb(glbAssetPaths_[imguiCarousel_.glbSelected])) {
                glbLoadedSelection_=imguiCarousel_.glbSelected;
                glbScale_=0.1f;
                glbPreview_=true;
                glbPlaced_=false;
                imguiCarousel_.voxenPaintEnabled=false;
                imguiCarousel_.glbStatus="Preview active: aim then trigger; right stick left/right changes size";
            } else {
                glbLoadedSelection_=-1;
                glbPreview_=false;
                imguiCarousel_.glbStatus="GLB load failed";
            }
        }
#endif
        if(imguiCarousel_.panoramaRescanRequest) {
            imguiCarousel_.panoramaRescanRequest=false;
            rescanPanoramaAssets();
        }
        imguiCarousel_.panoramaFiles=panoramaNames_;
#ifdef VOXEN_WITH_GLB_IMPORT
        // Never decode/downscale on the XR render thread. A single worker
        // decodes at a time; rapid selection changes discard stale results.
        if(panoramaDecodeFuture_.valid() &&
           panoramaDecodeFuture_.wait_for(std::chrono::seconds(0))==std::future_status::ready) {
            try {
                auto decoded=panoramaDecodeFuture_.get();
                if(panoramaDecodeIndex_==imguiCarousel_.panoramaSelected &&
                   panoramaDecodeIndex_>=0) {
                    if(loadPanoramaGpu(std::move(decoded))) {
                        panoramaFadeStart_=std::chrono::steady_clock::now();
                        panoramaFadeActive_=true;
                        imguiCarousel_.panoramaStatus="Panorama ready";
                    } else imguiCarousel_.panoramaStatus="Panorama GPU upload failed";
                }
            } catch(const std::exception& error) {
                if(panoramaDecodeIndex_==imguiCarousel_.panoramaSelected)
                    imguiCarousel_.panoramaStatus=error.what();
            }
            panoramaDecodeIndex_=-1;
        }
#endif
        if(imguiCarousel_.panoramaSelected!=panoramaDecodedSelection_) {
            panoramaDecodedSelection_=imguiCarousel_.panoramaSelected;
#ifdef VOXEN_WITH_GLB_IMPORT
            panoramaFadeActive_=false;
            if(device_)vkDeviceWaitIdle(device_);
            releasePanoramaGpu();
            imguiCarousel_.panoramaStatus=panoramaDecodedSelection_<0
                ? std::string{} : "Loading panorama in background...";
#endif
        }
#ifdef VOXEN_WITH_GLB_IMPORT
        const int selected=imguiCarousel_.panoramaSelected;
        if(selected>=0 && static_cast<std::size_t>(selected)<panoramaNames_.size() &&
           !panoramaTexture_.descriptor && !panoramaDecodeFuture_.valid() &&
           panoramaDecodeIndex_<0 &&
           imguiCarousel_.panoramaStatus=="Loading panorama in background...") {
            const auto file=panoramaRoot_/panoramaNames_[selected];
            VkPhysicalDeviceProperties props{};
            vkGetPhysicalDeviceProperties(physicalDevice_,&props);
            const int limit=static_cast<int>(std::min(props.limits.maxImageDimension2D,8192u));
            panoramaDecodeIndex_=selected;
            panoramaDecodeFuture_=std::async(std::launch::async,[file,limit] {
                return decodePanoramaImage(file,limit,16ull*1024ull*1024ull);
            });
        }
#endif
        imguiCarousel_.musicFiles=mp3Library_.titles();
        imguiCarousel_.musicError=mp3Library_.error();
        imguiCarousel_.savedSceneIds=sceneIds_;
        imguiCarousel_.savedSceneLabels=sceneLabels_;
        imguiCarousel_.sceneAnchorReady=portalManager_ && portalManager_->localized();
        imguiCarousel_.sceneImmersive=immersiveSceneActive_;
        imguiCarousel_.visiblePortalCount=visibleGalleryPortals().size();
        imguiCarousel_.scenePortalSelected=selectedPortalId_.has_value();
        voxen::ui::drawCarousel(imguiCarousel_);
#ifdef VOXEN_WITH_GLB_IMPORT
        if(imguiCarousel_.voxenPaintEnabled && glbPreview_)glbPreview_=false;
#endif
        if(imguiCarousel_.sceneSave) {
            imguiCarousel_.sceneSave=false;
            imguiCarousel_.sceneStatus=saveCurrentScene()?"Scene saved - DISPOSE available":"Save failed";
        }
        imguiCarousel_.savedSceneIds=sceneIds_;
        if(imguiCarousel_.sceneDelete) {
            imguiCarousel_.sceneDelete=false;
            const int index=imguiCarousel_.selectedScene;
            const bool valid=index>=0 &&
                static_cast<std::size_t>(index)<sceneIds_.size();
            const auto id=valid?sceneIds_[index]:voxen::gallery::SceneId{};
            imguiCarousel_.sceneStatus=valid && deleteSavedScene(id)?
                "Scene and its portals deleted":"Scene deletion failed";
            imguiCarousel_.savedSceneIds=sceneIds_;
            imguiCarousel_.savedSceneLabels=sceneLabels_;
        }
        if(imguiCarousel_.sceneEnter) {
            imguiCarousel_.sceneEnter=false;
            const int index=imguiCarousel_.selectedScene;
            imguiCarousel_.sceneStatus=index>=0 &&
                static_cast<std::size_t>(index)<sceneIds_.size() &&
                enterImmersiveScene(sceneIds_[index]) ?
                "Immersive scene entered":"Cannot enter scene";
        }
        if(imguiCarousel_.sceneLeave) {
            imguiCarousel_.sceneLeave=false;
            imguiCarousel_.sceneStatus=leaveImmersiveScene()?
                "Gallery restored (CPU scene instances released)":"Not in immersive mode";
        }
        if(imguiCarousel_.scenePlace) {
            imguiCarousel_.scenePlace=false;
            selectedSceneIndex_=imguiCarousel_.selectedScene;
            pendingGalleryFloorAction_=1;
            lastGalleryTriggerPressed_=true; // Require release after clicking DISPOSE.
            if(immersiveSceneActive_)leaveImmersiveScene();
            setEditingMode(false);
            imguiCarousel_.sceneStatus="GALLERY: move right controller, trigger to place";
        }
        if(imguiCarousel_.scenePortalCancel) {
            imguiCarousel_.scenePortalCancel=false;
            pendingGalleryFloorAction_=0;
            imguiCarousel_.sceneStatus="Floor action cancelled";
        }
        if(imguiCarousel_.scenePortalSelect) {
            imguiCarousel_.scenePortalSelect=false;
            pendingGalleryFloorAction_=2;
            imguiCarousel_.sceneStatus="Aim at a floor portal and press trigger";
        }
        if(imguiCarousel_.scenePortalMove) {
            imguiCarousel_.scenePortalMove=false;
            pendingGalleryFloorAction_=3;
            imguiCarousel_.sceneStatus="Aim at new tracked floor position and press trigger";
        }
        if(imguiCarousel_.scenePortalDelete) {
            imguiCarousel_.scenePortalDelete=false;
            imguiCarousel_.sceneStatus=deleteSelectedGalleryPortal()?
                "Portal removed (saved scene kept)":"Portal removal failed";
        }
        if(imguiCarousel_.sceneLoadEdit) {
            imguiCarousel_.sceneLoadEdit=false;
            const int index=imguiCarousel_.selectedScene;
            const bool valid=index>=0 && static_cast<size_t>(index)<sceneIds_.size();
            // CHARGER is an editor action only. In particular, do not
            // treat the built-in mechanical cannon as a walk-in portal.
            const bool ok=imguiCarousel_.editMode && !immersiveSceneActive_ &&
                          valid && loadSavedScene(sceneIds_[index]);
            imguiCarousel_.sceneStatus=ok?"Scene loaded in editor":"Cannot load selected scene in editor";
        }
        if(imguiCarousel_.sceneLoad) {
            imguiCarousel_.sceneLoad=false;
            if(imguiCarousel_.selectedScene>=0 &&
               static_cast<std::size_t>(imguiCarousel_.selectedScene)<sceneIds_.size())
                imguiCarousel_.sceneStatus=
                    (sceneIds_[imguiCarousel_.selectedScene]==mechanicalCannonSceneId_
                        ? enterImmersiveScene(sceneIds_[imguiCarousel_.selectedScene])
                        : loadSavedScene(sceneIds_[imguiCarousel_.selectedScene])) ?
                    "Scene loaded":"Load failed";
        }
        if(imguiCarousel_.voxenClear){
            voxens_.clear();
            lastVoxenValid_=false;
            imguiCarousel_.voxenClear=false;
        }
#ifdef VOXEN_WITH_GLB_IMPORT
        if(imguiCarousel_.glbRescan) {
            imguiCarousel_.glbRescan=false;
            rescanGlbAssets();
            imguiCarousel_.glbSelected=0;
            glbLoadedSelection_=-1;
            imguiCarousel_.glbStatus="Asset list refreshed";
        }
        if(imguiCarousel_.glbPlaceScene || imguiCarousel_.glbPlaceGallery) {
            const bool gallery=imguiCarousel_.glbPlaceGallery;
            imguiCarousel_.glbPlaceScene=false;
            imguiCarousel_.glbPlaceGallery=false;
            const auto i=imguiCarousel_.glbSelected;
            if(i<0 || static_cast<size_t>(i)>=glbAssetPaths_.size()) {
                imguiCarousel_.glbStatus="Select a GLB first";
            } else if(gallery && (!portalManager_ || !portalManager_->localized())) {
                imguiCarousel_.glbStatus="Gallery placement needs a localized Meta room";
            } else {
                const auto hit=gallery?galleryFloorHit():std::optional<voxen::gallery::FloorHit>{};
                if(gallery && !hit) {
                    imguiCarousel_.glbStatus="Aim controller at a tracked floor first";
                } else if(!gallery && !controllerTracked_[1]) {
                    imguiCarousel_.glbStatus="Right controller is not tracked";
                } else {
                    const auto& pos=controllerPoses_[1].position;
                    // Scene: initial position at right controller; gallery: floor hit.
                    if(!gallery) glbPlacement_={pos.x,pos.y,pos.z-0.3f};
                    else glbPlacement_=hit->localPose.positionMeters;
                    if(loadDemoGlb(glbAssetPaths_[i])) {
                        glbInGallery_=gallery;
                        glbPlaced_=true;
                        glbPreview_=false;
                        glbLoadedSelection_=i;
                        imguiCarousel_.glbStatus=gallery?"GLB placed on tracked gallery floor":"GLB placed in scene";
                    } else imguiCarousel_.glbStatus="GLB decode / Vulkan upload failed";
                }
            }
        }
#endif
        if(imguiCarousel_.musicRescan){
            imguiCarousel_.musicRescan=false;
            mp3Library_.shutdown();
            mp3Library_.refresh();
            imguiCarousel_.musicFile=0;
        }
        const bool procedural=imguiCarousel_.musicSource==0;
        const bool sceneAudio=(editingMode() || immersiveSceneActive_) && imguiCarousel_.musicPlaying;
        ambientMusic_.configure(imguiCarousel_.musicTrack,
            procedural&&sceneAudio,imguiCarousel_.musicVolume);
        mp3Library_.update(procedural?-1:imguiCarousel_.musicFile,
            !procedural&&sceneAudio,imguiCarousel_.musicVolume);
        // Visible cursor proves that ray → panel UV mapping reaches ImGui.
        if(aiming) {
            ImGui::GetForegroundDrawList()->AddCircleFilled(
                ImVec2(cursorX,cursorY),7.f,IM_COL32(100,255,150,255));
        }
        ImGui::Render();
        nativeMenuPage_=imguiCarousel_.page;
        // ImDrawData is now available for a future offscreen VkImage renderer.
        // For now the old phosphor GLSL panel is still rendered below.
    }
#endif
    if (!firstFrameLogged_) {
#ifdef __ANDROID__
        __android_log_print(ANDROID_LOG_INFO, "Voxen", "Rendering first stereo projection frame");
#else
        std::printf("[Voxen] Rendering first stereo projection frame\n");
#endif
    }
    // Advance physics once per stereo frame, never separately for each eye.
#ifdef VOXEN_WITH_IMGUI
    const auto physicsNow=std::chrono::steady_clock::now();
    const float physicsDt=lastPhysicsFrame_.time_since_epoch().count()==0 ? 0.f :
        std::clamp(std::chrono::duration<float>(physicsNow-lastPhysicsFrame_).count(),0.f,0.1f);
    lastPhysicsFrame_=physicsNow;
    if(imguiCarousel_.motionEnabled){
        physicsAccumulator_=std::min(physicsAccumulator_+physicsDt,0.1f);
        constexpr float step=1.f/60.f;
        for(int steps=0;physicsAccumulator_>=step && steps<6;++steps){
            advanceVoxenPhysics(step,imguiCarousel_.motionGravity,
                imguiCarousel_.motionBounce,
                imguiCarousel_.motionMode,imguiCarousel_.swarmRadius,
                imguiCarousel_.swarmStrength,imguiCarousel_.swarmSeparation,
                imguiCarousel_.collideRoomMesh);
            physicsAccumulator_-=step;
        }
    }else physicsAccumulator_=0.f;
#endif
    // Select one LOD per object before rendering either eye. Hysteresis
    // prevents thrashing at thresholds and guarantees identical stereo meshes.
    const XrVector3f center{
        0.5f*(views[0].pose.position.x+views[1].pose.position.x),
        0.5f*(views[0].pose.position.y+views[1].pose.position.y),
        0.5f*(views[0].pose.position.z+views[1].pose.position.z)
    };
    updateGalleryWalkIn(center);
    for(auto& v:voxens_){
        if(v.preset!=2)continue;
        const float dx=v.position[0]-center.x;
        const float dy=v.position[1]-center.y;
        const float dz=v.position[2]-center.z;
        const float distance=std::sqrt(dx*dx+dy*dy+dz*dz);
        const float apparent=v.scale/std::max(distance,0.15f);
        // Base switches at 0.24 / 0.11, with a 15% hysteresis band.
        if(v.lodLevel<0) v.lodLevel=apparent>=0.24f?0:(apparent>=0.11f?1:2);
        else if(v.lodLevel==0 && apparent<0.204f)v.lodLevel=1;
        else if(v.lodLevel==1){
            if(apparent>0.276f)v.lodLevel=0;
            else if(apparent<0.0935f)v.lodLevel=2;
        }else if(v.lodLevel==2 && apparent>0.1265f)v.lodLevel=1;
    }
    for (uint32_t eye = 0; eye < eyes_.size(); ++eye) {
        auto& swapchain = eyes_[eye];

        XrSwapchainImageAcquireInfo acquireInfo{XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
        uint32_t imageIndex = 0;
        checkXr(xrAcquireSwapchainImage(swapchain.handle, &acquireInfo, &imageIndex), "xrAcquireSwapchainImage");

        XrSwapchainImageWaitInfo waitInfo{XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};
        waitInfo.timeout = XR_INFINITE_DURATION;
        checkXr(xrWaitSwapchainImage(swapchain.handle, &waitInfo), "xrWaitSwapchainImage");

        renderImage(swapchain, imageIndex, views[eye]);

        XrSwapchainImageReleaseInfo releaseInfo{XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
        checkXr(xrReleaseSwapchainImage(swapchain.handle, &releaseInfo), "xrReleaseSwapchainImage");

        auto& projection = projectionViews[eye];
        projection = {XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW};
        projection.pose = views[eye].pose;
        projection.fov = views[eye].fov;
        projection.subImage.swapchain = swapchain.handle;
        projection.subImage.imageRect.offset = {0, 0};
        projection.subImage.imageRect.extent = {swapchain.width, swapchain.height};
        projection.subImage.imageArrayIndex = 0;
    }

    // Throttled diagnostic while the native Vulkan HUD is under construction.
    // GPU timestamp is measured per eye; this value is the last eye rendered.
    static auto lastStatsLog = std::chrono::steady_clock::now();
    const auto now = std::chrono::steady_clock::now();
    if (now - lastStatsLog >= std::chrono::seconds(2)) {
        lastStatsLog = now;
        uint32_t lodCounts[3]={};
        uint64_t displayedTriangles=0;
        for(const auto& object:voxens_){
            if(object.preset!=2)continue;
            const int lod=std::clamp(object.lodLevel,0,2);
            if(mandelbulbLods_[lod].indexCount==0)continue;
            ++lodCounts[lod];
            displayedTriangles+=mandelbulbLods_[lod].indexCount/3;
        }
#ifdef __ANDROID__
        if (gpuTimestampPool_ != VK_NULL_HANDLE && completedGpuSamples_ > 0)
            __android_log_print(ANDROID_LOG_INFO,"VoxenPerf","GPU last-eye %.2f ms",lastGpuMs_);
        else
            __android_log_print(ANDROID_LOG_INFO,"VoxenPerf","GPU N/A (timestamps unsupported)");
        __android_log_print(ANDROID_LOG_INFO,"VoxenPerf",
            "Mandelbulb LOD instances: L0=%u L1=%u L2=%u triangles/eye=%llu",
            lodCounts[0],lodCounts[1],lodCounts[2],
            static_cast<unsigned long long>(displayedTriangles));
#else
        if (gpuTimestampPool_ != VK_NULL_HANDLE && completedGpuSamples_ > 0)
            std::printf("[VoxenPerf] GPU last-eye %.2f ms\\n",lastGpuMs_);
        else
            std::printf("[VoxenPerf] GPU N/A (timestamps unsupported)\\n");
        std::printf("[VoxenPerf] Mandelbulb LOD instances: L0=%u L1=%u L2=%u triangles/eye=%llu\\n",
            lodCounts[0],lodCounts[1],lodCounts[2],
            static_cast<unsigned long long>(displayedTriangles));
#endif
    }
    if (!firstFrameLogged_) {
#ifdef __ANDROID__
        __android_log_print(ANDROID_LOG_INFO, "Voxen", "First stereo projection frame rendered");
#else
        std::printf("[Voxen] First stereo projection frame rendered\n");
#endif
        firstFrameLogged_ = true;
    }
}

void XrSwapchainRenderer::shutdown() noexcept {
#ifdef VOXEN_WITH_GLB_IMPORT
    // Join the worker before destroying renderer state and scene storage.
    if(panoramaDecodeFuture_.valid()) {
        panoramaDecodeFuture_.wait();
        try { (void)panoramaDecodeFuture_.get(); } catch(...) {}
    }
#endif
    mp3Library_.shutdown();
    ambientMusic_.stop();
    if (device_ != VK_NULL_HANDLE) vkDeviceWaitIdle(device_);
#ifdef VOXEN_WITH_IMGUI
    imguiSurface_.shutdown();
    if (imguiContextOwned_ && ImGui::GetCurrentContext() != nullptr) {
        ImGui::DestroyContext();
        imguiContextOwned_=false;
    }
#endif
    if (device_ != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(device_);
    }

    for (auto& eye : eyes_) {
        for (auto fb : eye.framebuffers) if (fb != VK_NULL_HANDLE) vkDestroyFramebuffer(device_, fb, nullptr);
        for (auto view : eye.views) if (view != VK_NULL_HANDLE) vkDestroyImageView(device_, view, nullptr);
        eye.framebuffers.clear(); eye.views.clear();
        if (eye.depthView != VK_NULL_HANDLE) { vkDestroyImageView(device_, eye.depthView, nullptr); eye.depthView = VK_NULL_HANDLE; }
        if (eye.depthImage != VK_NULL_HANDLE) { vkDestroyImage(device_, eye.depthImage, nullptr); eye.depthImage = VK_NULL_HANDLE; }
        if (eye.depthMemory != VK_NULL_HANDLE) { vkFreeMemory(device_, eye.depthMemory, nullptr); eye.depthMemory = VK_NULL_HANDLE; }
        if (eye.handle != XR_NULL_HANDLE) {
            xrDestroySwapchain(eye.handle);
            eye.handle = XR_NULL_HANDLE;
        }
        eye.images.clear();
        eye.layouts.clear();
    }

#ifdef VOXEN_WITH_GLB_IMPORT
    releasePanoramaGpu();
#endif
    if(panoramaPipeline_) { vkDestroyPipeline(device_,panoramaPipeline_,nullptr); panoramaPipeline_=VK_NULL_HANDLE; }
    if(panoramaPipelineLayout_) { vkDestroyPipelineLayout(device_,panoramaPipelineLayout_,nullptr); panoramaPipelineLayout_=VK_NULL_HANDLE; }
    if(panoramaSetLayout_) { vkDestroyDescriptorSetLayout(device_,panoramaSetLayout_,nullptr); panoramaSetLayout_=VK_NULL_HANDLE; }
    if(organicSpherePipeline_!=VK_NULL_HANDLE){vkDestroyPipeline(device_,organicSpherePipeline_,nullptr);organicSpherePipeline_=VK_NULL_HANDLE;}
    if(torusKnotPipeline_!=VK_NULL_HANDLE){vkDestroyPipeline(device_,torusKnotPipeline_,nullptr);torusKnotPipeline_=VK_NULL_HANDLE;}
    if(spikeBloomPipeline_!=VK_NULL_HANDLE){vkDestroyPipeline(device_,spikeBloomPipeline_,nullptr);spikeBloomPipeline_=VK_NULL_HANDLE;}
    if(jellyfishPipeline_!=VK_NULL_HANDLE){vkDestroyPipeline(device_,jellyfishPipeline_,nullptr);jellyfishPipeline_=VK_NULL_HANDLE;}
    if(earthwormPipeline_!=VK_NULL_HANDLE){vkDestroyPipeline(device_,earthwormPipeline_,nullptr);earthwormPipeline_=VK_NULL_HANDLE;}
    if(cloudPipeline_!=VK_NULL_HANDLE){vkDestroyPipeline(device_,cloudPipeline_,nullptr);cloudPipeline_=VK_NULL_HANDLE;}
    if(metaballsPipeline_!=VK_NULL_HANDLE){vkDestroyPipeline(device_,metaballsPipeline_,nullptr);metaballsPipeline_=VK_NULL_HANDLE;}
    if(voxenPlasmaPipeline_!=VK_NULL_HANDLE){vkDestroyPipeline(device_,voxenPlasmaPipeline_,nullptr);voxenPlasmaPipeline_=VK_NULL_HANDLE;}
    if(voxenSpheresPipeline_!=VK_NULL_HANDLE){vkDestroyPipeline(device_,voxenSpheresPipeline_,nullptr);voxenSpheresPipeline_=VK_NULL_HANDLE;}
    if(voxenMandelbulbPipeline_!=VK_NULL_HANDLE){vkDestroyPipeline(device_,voxenMandelbulbPipeline_,nullptr);voxenMandelbulbPipeline_=VK_NULL_HANDLE;}
    if(voxenMandelbulbLitPipeline_!=VK_NULL_HANDLE){vkDestroyPipeline(device_,voxenMandelbulbLitPipeline_,nullptr);voxenMandelbulbLitPipeline_=VK_NULL_HANDLE;}
#ifdef VOXEN_WITH_GLB_IMPORT
    releaseGlbGpu();
    if(glbPipeline_!=VK_NULL_HANDLE){vkDestroyPipeline(device_,glbPipeline_,nullptr);glbPipeline_=VK_NULL_HANDLE;}
    if(glbOriginalPipeline_!=VK_NULL_HANDLE){vkDestroyPipeline(device_,glbOriginalPipeline_,nullptr);glbOriginalPipeline_=VK_NULL_HANDLE;}
    if(glbTexturedPipeline_!=VK_NULL_HANDLE){vkDestroyPipeline(device_,glbTexturedPipeline_,nullptr);glbTexturedPipeline_=VK_NULL_HANDLE;}
    if(glbTexturePipelineLayout_!=VK_NULL_HANDLE){vkDestroyPipelineLayout(device_,glbTexturePipelineLayout_,nullptr);glbTexturePipelineLayout_=VK_NULL_HANDLE;}
    if(glbTextureSetLayout_!=VK_NULL_HANDLE){vkDestroyDescriptorSetLayout(device_,glbTextureSetLayout_,nullptr);glbTextureSetLayout_=VK_NULL_HANDLE;}
#endif
    for(auto& lod:mandelbulbLods_){
        if(lod.vertexBuffer!=VK_NULL_HANDLE)vkDestroyBuffer(device_,lod.vertexBuffer,nullptr);
        if(lod.indexBuffer!=VK_NULL_HANDLE)vkDestroyBuffer(device_,lod.indexBuffer,nullptr);
        if(lod.vertexMemory!=VK_NULL_HANDLE)vkFreeMemory(device_,lod.vertexMemory,nullptr);
        if(lod.indexMemory!=VK_NULL_HANDLE)vkFreeMemory(device_,lod.indexMemory,nullptr);
        lod={};
    }
    if(voxenAttractorPipeline_!=VK_NULL_HANDLE){vkDestroyPipeline(device_,voxenAttractorPipeline_,nullptr);voxenAttractorPipeline_=VK_NULL_HANDLE;}
    voxens_.clear();
    if (laserPipeline_ != VK_NULL_HANDLE) { vkDestroyPipeline(device_, laserPipeline_, nullptr); laserPipeline_ = VK_NULL_HANDLE; }
    if (liquidRoomPipeline_ != VK_NULL_HANDLE) { vkDestroyPipeline(device_,liquidRoomPipeline_,nullptr); liquidRoomPipeline_=VK_NULL_HANDLE; }
    if (solidWallPipeline_ != VK_NULL_HANDLE) { vkDestroyPipeline(device_,solidWallPipeline_,nullptr); solidWallPipeline_=VK_NULL_HANDLE; }
    if (meshWirePipeline_ != VK_NULL_HANDLE) { vkDestroyPipeline(device_,meshWirePipeline_,nullptr); meshWirePipeline_=VK_NULL_HANDLE; }
    if(meshDepthPipeline_!=VK_NULL_HANDLE){vkDestroyPipeline(device_,meshDepthPipeline_,nullptr);meshDepthPipeline_=VK_NULL_HANDLE;}
    if(matrixRainPipeline_!=VK_NULL_HANDLE){vkDestroyPipeline(device_,matrixRainPipeline_,nullptr);matrixRainPipeline_=VK_NULL_HANDLE;}
    if(heppGlowPipeline_!=VK_NULL_HANDLE){vkDestroyPipeline(device_,heppGlowPipeline_,nullptr);heppGlowPipeline_=VK_NULL_HANDLE;}
    if(phiFieldPipeline_!=VK_NULL_HANDLE){vkDestroyPipeline(device_,phiFieldPipeline_,nullptr);phiFieldPipeline_=VK_NULL_HANDLE;}
    if(soundDemoPipeline_!=VK_NULL_HANDLE){vkDestroyPipeline(device_,soundDemoPipeline_,nullptr);soundDemoPipeline_=VK_NULL_HANDLE;}
    if(midnightRidePipeline_!=VK_NULL_HANDLE){vkDestroyPipeline(device_,midnightRidePipeline_,nullptr);midnightRidePipeline_=VK_NULL_HANDLE;}
    if(seascapePipeline_!=VK_NULL_HANDLE){vkDestroyPipeline(device_,seascapePipeline_,nullptr);seascapePipeline_=VK_NULL_HANDLE;}
    if(octgramsPipeline_!=VK_NULL_HANDLE){vkDestroyPipeline(device_,octgramsPipeline_,nullptr);octgramsPipeline_=VK_NULL_HANDLE;}
    if(mandelbrotZoomPipeline_!=VK_NULL_HANDLE){vkDestroyPipeline(device_,mandelbrotZoomPipeline_,nullptr);mandelbrotZoomPipeline_=VK_NULL_HANDLE;}
    if(meshDepthBuffer_!=VK_NULL_HANDLE){vkDestroyBuffer(device_,meshDepthBuffer_,nullptr);meshDepthBuffer_=VK_NULL_HANDLE;}
    if(meshDepthMemory_!=VK_NULL_HANDLE){vkFreeMemory(device_,meshDepthMemory_,nullptr);meshDepthMemory_=VK_NULL_HANDLE;}
    meshDepthVertexCount_=0;
    if(meshWireBuffer_!=VK_NULL_HANDLE){vkDestroyBuffer(device_,meshWireBuffer_,nullptr);meshWireBuffer_=VK_NULL_HANDLE;}
    if(meshWireMemory_!=VK_NULL_HANDLE){vkFreeMemory(device_,meshWireMemory_,nullptr);meshWireMemory_=VK_NULL_HANDLE;}
    meshWireVertexCount_=0;
    if (carouselPipeline_ != VK_NULL_HANDLE) { vkDestroyPipeline(device_,carouselPipeline_,nullptr); carouselPipeline_=VK_NULL_HANDLE; }
    if (galleryPortalPipeline_ != VK_NULL_HANDLE) { vkDestroyPipeline(device_,galleryPortalPipeline_,nullptr); galleryPortalPipeline_=VK_NULL_HANDLE; }
    if (mechanicalCannonPipeline_ != VK_NULL_HANDLE) { vkDestroyPipeline(device_,mechanicalCannonPipeline_,nullptr); mechanicalCannonPipeline_=VK_NULL_HANDLE; }
#ifdef VOXEN_WITH_IMGUI
    if (imguiPanelPipeline_ != VK_NULL_HANDLE) {vkDestroyPipeline(device_,imguiPanelPipeline_,nullptr);imguiPanelPipeline_=VK_NULL_HANDLE;}
    if (imguiPanelPipelineLayout_ != VK_NULL_HANDLE) {vkDestroyPipelineLayout(device_,imguiPanelPipelineLayout_,nullptr);imguiPanelPipelineLayout_=VK_NULL_HANDLE;}
#endif
    if (pipelineLayout_ != VK_NULL_HANDLE) { vkDestroyPipelineLayout(device_, pipelineLayout_, nullptr); pipelineLayout_ = VK_NULL_HANDLE; }
    if (renderPass_ != VK_NULL_HANDLE) { vkDestroyRenderPass(device_, renderPass_, nullptr); renderPass_ = VK_NULL_HANDLE; }

    if (gpuTimestampPool_ != VK_NULL_HANDLE) {
        vkDestroyQueryPool(device_, gpuTimestampPool_, nullptr);
        gpuTimestampPool_ = VK_NULL_HANDLE;
    }
    if (fence_ != VK_NULL_HANDLE) {
        vkDestroyFence(device_, fence_, nullptr);
        fence_ = VK_NULL_HANDLE;
    }
    if (commandPool_ != VK_NULL_HANDLE) {
        vkDestroyCommandPool(device_, commandPool_, nullptr);
        commandPool_ = VK_NULL_HANDLE;
    }

    commandBuffer_ = VK_NULL_HANDLE;
    queue_ = VK_NULL_HANDLE;
    device_ = VK_NULL_HANDLE;
    physicalDevice_ = VK_NULL_HANDLE;
    depthFormat_ = VK_FORMAT_UNDEFINED;
    session_ = XR_NULL_HANDLE;
}

} // namespace voxen::render
