#pragma once

#include <array>
#include <string>
#include <voxen/scene/EnvironmentSurface.hpp>

#include <vulkan/vulkan.h>
#include <openxr/openxr.h>
#ifdef __ANDROID__
#include <jni.h>
#include <openxr/openxr_platform.h>
#endif
#include <voxen/xr/MetaSceneProvider.hpp>

namespace voxen::render { class XrSwapchainRenderer; }

namespace voxen::xr {

class XrSession final {
public:
    XrSession() = default;
    ~XrSession();

    XrSession(const XrSession&) = delete;
    XrSession& operator=(const XrSession&) = delete;

    void initialize(
        XrInstance instance,
        XrSystemId systemId,
        VkInstance vkInstance,
        VkPhysicalDevice physicalDevice,
        VkDevice device,
        uint32_t queueFamilyIndex,
        bool enablePassthrough = false,
        bool enableScene = false,
        bool scenePermissionGranted = true,
        bool enableVideoSurface = false);
    void shutdown() noexcept;

    // Drains runtime events. Returns false when the runtime asks the app to exit.
    bool pollEvents();

    // Runs the OpenXR frame lifecycle. Rendering layers are intentionally empty
    // until swapchains and the first stereo renderer are added.
    void frame(voxen::render::XrSwapchainRenderer& renderer);
    void requestSceneRefresh();
#ifdef __ANDROID__
    bool openVideoSurface(const std::string& path, JavaVM* vm, jobject activity,
                          const voxen::scene::EnvironmentSurface* target = nullptr);
    void closeVideoSurface() noexcept;
#endif
    void setScenePermissionGranted(bool granted);

    [[nodiscard]] ::XrSession handle() const noexcept { return session_; }
    [[nodiscard]] XrSpace localSpace() const noexcept { return localSpace_; }
    [[nodiscard]] XrSessionState state() const noexcept { return state_; }
    [[nodiscard]] bool running() const noexcept { return running_; }

private:
    void handleSessionStateChanged(const XrEventDataSessionStateChanged& event);
    void initializePassthrough();
    void initializeActions();
    void syncActions();
    void destroyPassthrough() noexcept;
    void initializeHandTracking();
    void updateHandTracking(voxen::render::XrSwapchainRenderer& renderer, XrTime time);

    std::array<XrHandTrackerEXT,2> handTrackers_{XR_NULL_HANDLE,XR_NULL_HANDLE};
    PFN_xrCreateHandTrackerEXT createHandTracker_=nullptr;
    PFN_xrDestroyHandTrackerEXT destroyHandTracker_=nullptr;
    PFN_xrLocateHandJointsEXT locateHandJoints_=nullptr;
    XrInstance instance_ = XR_NULL_HANDLE;
#ifdef __ANDROID__
    XrSwapchain videoSwapchain_ = XR_NULL_HANDLE;
    jobject videoPlayer_ = nullptr;
    JavaVM* videoJavaVm_ = nullptr;
    PFN_xrCreateSwapchainAndroidSurfaceKHR createVideoSurface_ = nullptr;
#endif
    bool videoSurfaceSupported_ = false;
    bool videoPlaneActive_ = false;
    voxen::scene::EnvironmentSurface videoPlane_{};
    ::XrSession session_ = XR_NULL_HANDLE;
    XrSpace stageSpace_ = XR_NULL_HANDLE;
    XrSpace localSpace_ = XR_NULL_HANDLE;
    XrActionSet actionSet_ = XR_NULL_HANDLE;
    XrAction selectAction_ = XR_NULL_HANDLE;
    XrAction aimAction_ = XR_NULL_HANDLE;
    XrAction thumbstickAction_ = XR_NULL_HANDLE;
    XrAction leftMenuAction_ = XR_NULL_HANDLE;
    XrAction leftYAction_ = XR_NULL_HANDLE;
    XrAction rightDeleteAction_ = XR_NULL_HANDLE;
    bool menuPressed_ = false;
    bool yPressed_ = false;
    bool rightDeletePressed_ = false;
    bool rightDeletePending_ = false;
    bool menuTogglePending_ = false;
    std::array<XrSpace, 2> aimSpaces_{XR_NULL_HANDLE, XR_NULL_HANDLE};
    std::array<XrPath, 2> handPaths_{};
    std::array<bool, 2> selectionPressed_{};
    std::array<XrVector2f, 2> thumbstickValues_{};
    XrSessionState state_ = XR_SESSION_STATE_UNKNOWN;
    bool running_ = false;
    bool enablePassthrough_ = false;
    bool enableScene_ = false;
    bool scenePermissionGranted_ = true;
    MetaSceneProvider metaScene_{};
    XrPassthroughFB passthrough_ = XR_NULL_HANDLE;
    XrPassthroughLayerFB passthroughLayer_ = XR_NULL_HANDLE;
    PFN_xrCreatePassthroughFB createPassthrough_ = nullptr;
    PFN_xrDestroyPassthroughFB destroyPassthrough_ = nullptr;
    PFN_xrPassthroughStartFB startPassthrough_ = nullptr;
    PFN_xrPassthroughPauseFB pausePassthrough_ = nullptr;
    PFN_xrCreatePassthroughLayerFB createLayer_ = nullptr;
    PFN_xrDestroyPassthroughLayerFB destroyLayer_ = nullptr;
    PFN_xrPassthroughLayerSetStyleFB setLayerStyle_ = nullptr;
};

} // namespace voxen::xr
