#pragma once

#include <array>
#include <chrono>
#include <cstdint>
#include <vector>

#include <vulkan/vulkan.h>
#include <openxr/openxr.h>
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
        ::XrSession session,
        VkDevice device,
        VkPhysicalDevice physicalDevice,
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

    ::XrSession session_ = XR_NULL_HANDLE;
    VkDevice device_ = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;
    VkFormat depthFormat_ = VK_FORMAT_UNDEFINED;
    VkQueue queue_ = VK_NULL_HANDLE;
    uint32_t queueFamilyIndex_ = 0;
    VkCommandPool commandPool_ = VK_NULL_HANDLE;
    VkCommandBuffer commandBuffer_ = VK_NULL_HANDLE;
    VkFence fence_ = VK_NULL_HANDLE;
    VkFormat colorFormat_ = VK_FORMAT_UNDEFINED;
    VkRenderPass renderPass_ = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout_ = VK_NULL_HANDLE;
    VkPipeline pipeline_ = VK_NULL_HANDLE;
    VkPipeline organicaPipeline_ = VK_NULL_HANDLE;
    VkPipeline fractalPipeline_ = VK_NULL_HANDLE;
    std::array<EyeSwapchain, 2> eyes_{};
    bool firstFrameLogged_ = false;
    std::chrono::steady_clock::time_point animationStart_ = std::chrono::steady_clock::now();
};

} // namespace voxen::render
