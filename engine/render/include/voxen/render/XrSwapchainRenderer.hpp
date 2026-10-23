#pragma once

#include <array>
#include <chrono>
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
#include <voxen/scene/SceneRepository.hpp>
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
    bool saveCurrentScene();
    bool loadSavedScene(const voxen::gallery::SceneId& id);
    void setLocalizedGalleryRoom(std::optional<voxen::gallery::LocalizedRoom> room);
    [[nodiscard]] std::vector<voxen::gallery::VisiblePortal> visibleGalleryPortals() const;
    bool placeGalleryPortalFromRightController();
    bool enterImmersiveScene(const voxen::gallery::SceneId& id);
    bool leaveImmersiveScene();
    [[nodiscard]] std::vector<voxen::gallery::SceneId> savedSceneIds() const;
    void toggleUiVisibility() noexcept {
#ifdef VOXEN_WITH_IMGUI
        imguiCarousel_.visible=!imguiCarousel_.visible;
        if(!imguiCarousel_.visible)panelAimed_=false;
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
    VkPipeline mandelbrotZoomPipeline_ = VK_NULL_HANDLE;
    VkBuffer meshWireBuffer_ = VK_NULL_HANDLE;
    VkBuffer meshDepthBuffer_ = VK_NULL_HANDLE;
    VkDeviceMemory meshDepthMemory_ = VK_NULL_HANDLE;
    uint32_t meshDepthVertexCount_ = 0;
    VkDeviceMemory meshWireMemory_ = VK_NULL_HANDLE;
    uint32_t meshWireVertexCount_ = 0;
    XrPosef meshWirePose_{};
    struct CollisionTriangle { std::array<float,3> a,b,c; };
    std::vector<CollisionTriangle> roomCollisionTriangles_;
    // 0.5m uniform grid: broad-phase candidate triangles, built on mesh updates.
    std::map<std::array<int,3>,std::vector<uint32_t>> roomCollisionGrid_;
    VkPipeline carouselPipeline_ = VK_NULL_HANDLE;
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
    std::unique_ptr<voxen::gallery::SceneRepository> sceneRepository_;
    std::unique_ptr<voxen::gallery::PortalManager> portalManager_;
    // Preview/scene mode; global Vulkan pipelines are shared, not freed here.
    bool immersiveSceneActive_ = false;
    std::vector<voxen::gallery::SceneId> sceneIds_;
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
    std::array<XrPosef, 2> controllerPoses_{};
    std::array<bool, 2> controllerTracked_{};
    bool firstFrameLogged_ = false;
    std::chrono::steady_clock::time_point animationStart_ = std::chrono::steady_clock::now();
};

} // namespace voxen::render
