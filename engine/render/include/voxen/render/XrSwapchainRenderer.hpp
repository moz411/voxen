#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include <jni.h>
#include <vulkan/vulkan.h>
#include <openxr/openxr.h>
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
        ::XrSession session,
        VkDevice device,
        VkQueue queue,
        uint32_t queueFamilyIndex);
    void shutdown() noexcept;

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
    };

    void createCommandResources();
    void createSwapchains(XrInstance instance, XrSystemId systemId, ::XrSession session);
    void clearImage(VkImage image, int32_t width, int32_t height, uint32_t eyeIndex);

    ::XrSession session_ = XR_NULL_HANDLE;
    VkDevice device_ = VK_NULL_HANDLE;
    VkQueue queue_ = VK_NULL_HANDLE;
    uint32_t queueFamilyIndex_ = 0;
    VkCommandPool commandPool_ = VK_NULL_HANDLE;
    VkCommandBuffer commandBuffer_ = VK_NULL_HANDLE;
    VkFence fence_ = VK_NULL_HANDLE;
    VkFormat colorFormat_ = VK_FORMAT_UNDEFINED;
    std::array<EyeSwapchain, 2> eyes_{};
};

} // namespace voxen::render
