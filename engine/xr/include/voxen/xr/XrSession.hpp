#pragma once

#include <array>

#include <openxr/openxr.h>
#include <vulkan/vulkan.h>

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
        uint32_t queueFamilyIndex);
    void shutdown() noexcept;

    // Drains runtime events. Returns false when the runtime asks the app to exit.
    bool pollEvents();

    // Runs the OpenXR frame lifecycle. Rendering layers are intentionally empty
    // until swapchains and the first stereo renderer are added.
    void frame(voxen::render::XrSwapchainRenderer& renderer);

    [[nodiscard]] ::XrSession handle() const noexcept { return session_; }
    [[nodiscard]] XrSpace localSpace() const noexcept { return localSpace_; }
    [[nodiscard]] XrSessionState state() const noexcept { return state_; }
    [[nodiscard]] bool running() const noexcept { return running_; }

private:
    void handleSessionStateChanged(const XrEventDataSessionStateChanged& event);

    XrInstance instance_ = XR_NULL_HANDLE;
    ::XrSession session_ = XR_NULL_HANDLE;
    XrSpace localSpace_ = XR_NULL_HANDLE;
    XrSessionState state_ = XR_SESSION_STATE_UNKNOWN;
    bool running_ = false;
};

} // namespace voxen::xr
