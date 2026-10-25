#pragma once

#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <vector>
#include <map>
#include <utility>

#include <vulkan/vulkan.h>
#include <openxr/openxr.h>
#include <voxen/audio/AmbientMusic.hpp>
#include <voxen/audio/Mp3Library.hpp>
#include <voxen/scene/EnvironmentSurface.hpp>
#include <voxen/scene/RuntimeSnapshot.hpp>
#include <voxen/scene/MechanicalCannon.hpp>
#include <voxen/scene/SceneRepository.hpp>
#include <unordered_map>
#include <chrono>
#include <voxen/scene/PortalManager.hpp>
#include <memory>
#include <filesystem>
#ifdef VOXEN_WITH_IMGUI
#include <voxen/ui/CarouselUi.hpp>
#include <voxen/ui/ImGuiSurface.hpp>
#endif
#ifdef _WIN32
#include <windows.h>
#include <unknwn.h>
#endif
#ifdef __ANDROID__
#include <jni.h>
#endif
#include <openxr/openxr_platform.h>

namespace voxen::render {

class XrSwapchainRenderer final {
public:
    XrSwapchainRenderer() = default;
    ~XrSwapchainRenderer();

    XrSwapchainRenderer(const XrSwapchainRenderer&) = delete;
    XrSwapchainRenderer& operator=(const XrSwapchainRenderer&) = delete;

    void initialize(
        XrInstance instance,
        XrSystemId systemId,
        VkInstance vulkanInstance,
        ::XrSession session,
        VkDevice device,
        VkPhysicalDevice physicalDevice,
        VkQueue queue,
        uint32_t queueFamilyIndex);
    void shutdown() noexcept;
    void configureSceneStorage(const std::filesystem::path& root);
#ifdef VOXEN_WITH_GLB_IMPORT
    // Explicit prototype import: app-private assets/demo.glb.
    bool loadDemoGlb(const std::filesystem::path& file);
#endif
    bool saveCurrentScene();
    bool loadSavedScene(const voxen::gallery::SceneId& id);
    bool deleteSavedScene(const voxen::gallery::SceneId& id);
    void setLocalizedGalleryRoom(std::optional<voxen::gallery::LocalizedRoom> room);
    [[nodiscard]] std::vector<voxen::gallery::VisiblePortal> visibleGalleryPortals() const;
    bool placeGalleryPortalFromRightController();
    bool selectGalleryPortalFromRightController();
    bool moveSelectedGalleryPortalFromRightController();
    bool deleteSelectedGalleryPortal();
    bool enterImmersiveScene(const voxen::gallery::SceneId& id);
    bool leaveImmersiveScene();
    [[nodiscard]] std::vector<voxen::gallery::SceneId> savedSceneIds() const;
    // Left controller MENU switches between gallery and the private editor.
    void toggleUiVisibility() noexcept { setEditingMode(!editingMode()); }
    void setEditingMode(bool enabled) noexcept;
    void requestGalleryDelete();
    [[nodiscard]] bool editingMode() const noexcept {
#ifdef VOXEN_WITH_IMGUI
        return imguiCarousel_.editMode;
#else
        return false;
#endif
    }
#ifdef __ANDROID__
    void configureMusicJavaVm(JavaVM* vm) {mp3Library_.initialize(vm);}
#endif
    // Provider output is empty until Meta OpenXR scene acquisition succeeds.
    struct MeshGeometry { XrPosef pose{}; const std::vector<XrVector3f>* vertices=nullptr; const std::vector<uint32_t>* indices=nullptr; };
    void setTrackedFloorHeight(float height) noexcept { trackedFloorY_=height; hasTrackedFloor_=true; }
    void setSceneMeshes(const std::vector<MeshGeometry>& meshes);
    void setEnvironmentSurfaces(std::vector<voxen::scene::EnvironmentSurface> surfaces) {
        environmentSurfaces_.replace(std::move(surfaces));
    }
    [[nodiscard]] std::size_t trackedSurfaceCount() const noexcept {
        return environmentSurfaces_.surfaces().size();
    }
    [[nodiscard]] float lastGpuMs() const noexcept { return lastGpuMs_; }
    [[nodiscard]] bool gpuTimingAvailable() const noexcept { return gpuTimestampPool_ != VK_NULL_HANDLE; }
    [[nodiscard]] bool carouselVisible() const noexcept {
#ifdef VOXEN_WITH_IMGUI
        return imguiCarousel_.visible;
#else
        return true;
#endif
    }
    struct HolographicHand {
        bool tracked=false;
        std::array<XrVector3f,26> joints{};
        std::array<float,26> radii{};
        XrVector3f palmVelocity{};
    };
    void setHolographicHands(const std::array<HolographicHand,2>& hands) {
        const auto now=std::chrono::steady_clock::now();
        const float dt=std::chrono::duration<float>(now-lastHandSample_).count();
        const bool validDt=lastHandSample_.time_since_epoch().count()!=0 &&
                           dt>0.001f && dt<0.1f;
        for(std::size_t i=0;i<hands.size();++i) {
            auto next=hands[i];
            if(next.tracked && holographicHands_[i].tracked && validDt) {
                const auto& a=next.joints[XR_HAND_JOINT_PALM_EXT];
                const auto& b=holographicHands_[i].joints[XR_HAND_JOINT_PALM_EXT];
                // Low-pass filter tracker jitter, and cap gesture speed.
                const float dx=(a.x-b.x)/dt,dy=(a.y-b.y)/dt,dz=(a.z-b.z)/dt;
                const float speed=std::sqrt(dx*dx+dy*dy+dz*dz);
                const float factor=speed>3.f?3.f/speed:1.f;
                constexpr float smoothing=0.45f;
                const auto& old=holographicHands_[i].palmVelocity;
                next.palmVelocity={
                    smoothing*dx*factor+(1.f-smoothing)*old.x,
                    smoothing*dy*factor+(1.f-smoothing)*old.y,
                    smoothing*dz*factor+(1.f-smoothing)*old.z
                };
            }
            // Newly tracked hands start with zero velocity, avoiding spikes.
            holographicHands_[i]=next;
        }
        lastHandSample_=now;
    }
    void updateControllerInteraction(const std::array<XrPosef, 2>& poses,
                                     const std::array<bool, 2>& tracked,
                                     const std::array<bool, 2>& pressed,
                                     const std::array<XrVector2f, 2>& thumbsticks);

    // Acquires, clears and releases both eye images, then fills projection views.
    void render(
        const std::array<XrView, 2>& views,
        std::array<XrCompositionLayerProjectionView, 2>& projectionViews);

private:
    struct EyeSwapchain {
        XrSwapchain handle = XR_NULL_HANDLE;
        int32_t width = 0;
        int32_t height = 0;
        std::vector<XrSwapchainImageVulkan2KHR> images;
        std::vector<VkImageLayout> layouts;
        std::vector<VkImageView> views;
        std::vector<VkFramebuffer> framebuffers;
        VkImage depthImage = VK_NULL_HANDLE;
        VkDeviceMemory depthMemory = VK_NULL_HANDLE;
        VkImageView depthView = VK_NULL_HANDLE;
    };

    void createCommandResources();
    void createSwapchains(XrInstance instance, XrSystemId systemId, ::XrSession session);
    void createPipeline();
    void createFramebuffers();
    void renderImage(EyeSwapchain& eye, uint32_t imageIndex, const XrView& view);

    voxen::audio::AmbientMusic ambientMusic_{};
    voxen::audio::Mp3Library mp3Library_{};
    ::XrSession session_ = XR_NULL_HANDLE;
    VkDevice device_ = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;
    VkFormat depthFormat_ = VK_FORMAT_UNDEFINED;
    VkQueue queue_ = VK_NULL_HANDLE;
    uint32_t queueFamilyIndex_ = 0;
    VkCommandPool commandPool_ = VK_NULL_HANDLE;
    VkCommandBuffer commandBuffer_ = VK_NULL_HANDLE;
    VkFence fence_ = VK_NULL_HANDLE;
    VkQueryPool gpuTimestampPool_ = VK_NULL_HANDLE;
    float timestampPeriodNs_ = 0.0f;
    float lastGpuMs_ = 0.0f;
    uint64_t completedGpuSamples_ = 0;
    VkFormat colorFormat_ = VK_FORMAT_UNDEFINED;
    VkRenderPass renderPass_ = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout_ = VK_NULL_HANDLE;
    VkPipeline voxenPlasmaPipeline_ = VK_NULL_HANDLE;
    VkPipeline voxenSpheresPipeline_ = VK_NULL_HANDLE;
    VkPipeline voxenMandelbulbPipeline_ = VK_NULL_HANDLE;
    VkPipeline voxenMandelbulbLitPipeline_ = VK_NULL_HANDLE;
#ifdef VOXEN_WITH_GLB_IMPORT
    VkPipeline glbPipeline_ = VK_NULL_HANDLE;
    std::filesystem::path pendingDemoGlb_;
    std::filesystem::path glbAssetRoot_;
    std::vector<std::filesystem::path> glbAssetPaths_;
    std::vector<std::string> glbAssetNames_;
    void rescanGlbAssets();
    std::array<float,3> glbPlacement_{0.f,0.f,-1.5f};
    bool glbInGallery_=false;
    bool glbPlaced_=false;
    struct GlbGpuPrimitive {
        VkBuffer vertexBuffer=VK_NULL_HANDLE;
        VkDeviceMemory vertexMemory=VK_NULL_HANDLE;
        VkBuffer indexBuffer=VK_NULL_HANDLE;
        VkDeviceMemory indexMemory=VK_NULL_HANDLE;
        uint32_t indexCount=0;
        std::array<float,16> worldMatrix{};
    };
    std::vector<GlbGpuPrimitive> glbPrimitives_;
    void releaseGlbGpu() noexcept;
#endif
    struct MandelbulbLod {
        VkBuffer vertexBuffer=VK_NULL_HANDLE;
        VkDeviceMemory vertexMemory=VK_NULL_HANDLE;
        VkBuffer indexBuffer=VK_NULL_HANDLE;
        VkDeviceMemory indexMemory=VK_NULL_HANDLE;
        uint32_t vertexCount=0;
        uint32_t indexCount=0;
    };
    std::array<MandelbulbLod,3> mandelbulbLods_{};
    VkPipeline voxenAttractorPipeline_ = VK_NULL_HANDLE;
    VkPipeline liquidRoomPipeline_ = VK_NULL_HANDLE;
    VkPipeline solidWallPipeline_ = VK_NULL_HANDLE;
    VkPipeline meshWirePipeline_ = VK_NULL_HANDLE;
    VkPipeline meshDepthPipeline_ = VK_NULL_HANDLE;
    VkPipeline matrixRainPipeline_ = VK_NULL_HANDLE;
    VkPipeline heppGlowPipeline_ = VK_NULL_HANDLE;
    VkPipeline phiFieldPipeline_ = VK_NULL_HANDLE;
    VkPipeline soundDemoPipeline_ = VK_NULL_HANDLE;
    VkPipeline midnightRidePipeline_ = VK_NULL_HANDLE;
    VkPipeline seascapePipeline_ = VK_NULL_HANDLE;
    VkPipeline octgramsPipeline_ = VK_NULL_HANDLE;
    VkPipeline mandelbrotZoomPipeline_ = VK_NULL_HANDLE;
    VkBuffer meshWireBuffer_ = VK_NULL_HANDLE;
    VkBuffer meshDepthBuffer_ = VK_NULL_HANDLE;
    VkDeviceMemory meshDepthMemory_ = VK_NULL_HANDLE;
    uint32_t meshDepthVertexCount_ = 0;
    VkDeviceMemory meshWireMemory_ = VK_NULL_HANDLE;
    uint32_t meshWireVertexCount_ = 0;
    XrPosef meshWirePose_{};
    const std::vector<XrVector3f>* lastCollisionMeshVertices_ = nullptr;
    const std::vector<uint32_t>* lastCollisionMeshIndices_ = nullptr;
    struct CollisionTriangle { std::array<float,3> a,b,c; };
    std::vector<CollisionTriangle> roomCollisionTriangles_;
    // 0.5m uniform grid: broad-phase candidate triangles, built on mesh updates.
    std::map<std::array<int,3>,std::vector<uint32_t>> roomCollisionGrid_;
    VkPipeline carouselPipeline_ = VK_NULL_HANDLE;
    VkPipeline galleryPortalPipeline_ = VK_NULL_HANDLE;
    VkPipeline mechanicalCannonPipeline_ = VK_NULL_HANDLE;
    bool mechanicalCannonPreview_ = false;
    std::chrono::steady_clock::time_point mechanicalCannonStart_{};
    static constexpr const char* mechanicalCannonSceneId_ = "voxen-mechanical-cannon-v1";
    int nativeMenuPage_ = 0;
    bool nativeMenuStickLatched_ = false;
#ifdef VOXEN_WITH_IMGUI
    voxen::ui::CarouselState imguiCarousel_{};
    bool imguiContextOwned_ = false;
    bool uiTrigger_[2]{};
    bool panelAimed_ = false;
    float panelHitDistance_ = 0.0f;
    voxen::ui::ImGuiSurface imguiSurface_{};
    VkPipeline imguiPanelPipeline_ = VK_NULL_HANDLE;
    VkPipelineLayout imguiPanelPipelineLayout_ = VK_NULL_HANDLE;
    std::chrono::steady_clock::time_point lastUiFrame_{};
#endif
    VkPipeline organicSpherePipeline_ = VK_NULL_HANDLE;
    VkPipeline torusKnotPipeline_ = VK_NULL_HANDLE;
    VkPipeline spikeBloomPipeline_ = VK_NULL_HANDLE;
    VkPipeline jellyfishPipeline_ = VK_NULL_HANDLE;
    VkPipeline earthwormPipeline_ = VK_NULL_HANDLE;
    VkPipeline cloudPipeline_ = VK_NULL_HANDLE;
    VkPipeline metaballsPipeline_ = VK_NULL_HANDLE;
    VkPipeline laserPipeline_ = VK_NULL_HANDLE;
    std::array<EyeSwapchain, 2> eyes_{};
    voxen::scene::EnvironmentSurfaceRegistry environmentSurfaces_{};
    struct PlasmaVoxen {
        int preset=0;
        std::array<float,3> position{};
        float scale=0.18f;
        float hue=0.52f;
        float density=1.0f;
        float speed=1.0f;
        std::array<float,3> velocity{};
        int material=0;
        int lodLevel=-1; // Chosen once per stereo frame using both eye positions
    };
    std::vector<PlasmaVoxen> voxens_;
    std::vector<PlasmaVoxen> editorVoxens_; // Suspended while gallery/immersion runs.
#ifdef VOXEN_WITH_IMGUI
    voxen::ui::CarouselState editorSettings_{};
#endif
    std::unique_ptr<voxen::gallery::SceneRepository> sceneRepository_;
    std::unique_ptr<voxen::gallery::PortalManager> portalManager_;
    // Preview/scene mode; global Vulkan pipelines are shared, not freed here.
    bool immersiveSceneActive_ = false;
    std::optional<voxen::gallery::InstanceId> galleryEntryCandidate_;
    std::optional<voxen::gallery::InstanceId> immersivePortalId_;
    std::chrono::steady_clock::time_point galleryExitSince_{};
    std::chrono::steady_clock::time_point galleryEntrySince_{};
    bool galleryEntryArmed_ = true;
    void updateGalleryWalkIn(const XrVector3f& headPosition);
    std::array<HolographicHand,2> holographicHands_{};
    std::chrono::steady_clock::time_point lastHandSample_{};
    std::optional<voxen::gallery::InstanceId> selectedPortalId_;
    int pendingGalleryFloorAction_ = 0; // 1 place, 2 select, 3 move
    bool lastGalleryTriggerPressed_ = false;
    bool galleryDragging_ = false;
    float galleryGrabDistance_ = 0.5f;
    voxen::gallery::Pose galleryDraggedPose_{};
    std::optional<voxen::gallery::InstanceId> galleryHoveredId_;
    std::chrono::steady_clock::time_point galleryDeleteArmedUntil_{};
    [[nodiscard]] std::optional<voxen::gallery::FloorHit> galleryFloorHit() const;
    std::vector<voxen::gallery::SceneId> sceneIds_;
    std::unordered_map<voxen::gallery::SceneId, voxen::gallery::Preview> scenePreviewCache_;
    std::vector<std::string> sceneLabels_;
    void refreshSceneIndex();
    int selectedSceneIndex_ = 0;
    std::string sceneStatus_;
    std::chrono::steady_clock::time_point lastPhysicsFrame_{};
    float physicsAccumulator_=0.f;
    float trackedFloorY_=0.f;
    bool hasTrackedFloor_=false;
    void advanceVoxenPhysics(float dt, bool gravity, float bounce,
        int mode, float swarmRadius, float swarmStrength, float swarmSeparation, bool collideRoomMesh);
    std::array<float,3> lastVoxenPos_{};
    bool lastVoxenValid_=false;
    bool editorPresetStickLatched_=false;
    bool editorScaleStickLatched_=false;
    std::array<XrPosef, 2> controllerPoses_{};
    std::array<bool, 2> controllerTracked_{};
    bool firstFrameLogged_ = false;
    std::chrono::steady_clock::time_point animationStart_ = std::chrono::steady_clock::now();
};

} // namespace voxen::render
