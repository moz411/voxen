#include <voxen/render/XrSwapchainRenderer.hpp>

#include <openxr/openxr_platform.h>

#include <algorithm>
#include <array>
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

XrSwapchainRenderer::~XrSwapchainRenderer() {
    shutdown();
}

void XrSwapchainRenderer::initialize(
    XrInstance instance,
    XrSystemId systemId,
    ::XrSession session,
    VkDevice device,
    VkQueue queue,
    uint32_t queueFamilyIndex) {
    session_ = session;
    device_ = device;
    queue_ = queue;
    queueFamilyIndex_ = queueFamilyIndex;

    createCommandResources();
    createSwapchains(instance, systemId, session);
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
        checkXr(
            xrEnumerateSwapchainImages(
                target.handle,
                imageCount,
                &imageCount,
                reinterpret_cast<XrSwapchainImageBaseHeader*>(target.images.data())),
            "xrEnumerateSwapchainImages");
    }
}

void XrSwapchainRenderer::clearImage(VkImage image, int32_t, int32_t, uint32_t eyeIndex) {
    checkVk(vkResetFences(device_, 1, &fence_), "vkResetFences");
    checkVk(vkResetCommandBuffer(commandBuffer_, 0), "vkResetCommandBuffer");

    VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    checkVk(vkBeginCommandBuffer(commandBuffer_, &begin), "vkBeginCommandBuffer");

    VkImageMemoryBarrier toTransfer{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    toTransfer.srcAccessMask = 0;
    toTransfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    toTransfer.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    toTransfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    toTransfer.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toTransfer.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toTransfer.image = image;
    toTransfer.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    toTransfer.subresourceRange.levelCount = 1;
    toTransfer.subresourceRange.layerCount = 1;

    vkCmdPipelineBarrier(
        commandBuffer_,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        0, 0, nullptr, 0, nullptr, 1, &toTransfer);

    VkClearColorValue clear{};
    clear.float32[0] = eyeIndex == 0 ? 0.08F : 0.02F;
    clear.float32[1] = 0.12F;
    clear.float32[2] = eyeIndex == 0 ? 0.20F : 0.26F;
    clear.float32[3] = 1.0F;
    vkCmdClearColorImage(
        commandBuffer_,
        image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        &clear,
        1,
        &toTransfer.subresourceRange);

    VkImageMemoryBarrier toColor{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    toColor.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    toColor.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
    toColor.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    toColor.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    toColor.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toColor.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toColor.image = image;
    toColor.subresourceRange = toTransfer.subresourceRange;

    vkCmdPipelineBarrier(
        commandBuffer_,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        0, 0, nullptr, 0, nullptr, 1, &toColor);

    checkVk(vkEndCommandBuffer(commandBuffer_), "vkEndCommandBuffer");

    VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &commandBuffer_;
    checkVk(vkQueueSubmit(queue_, 1, &submit, fence_), "vkQueueSubmit");
    checkVk(vkWaitForFences(device_, 1, &fence_, VK_TRUE, UINT64_MAX), "vkWaitForFences");
}

void XrSwapchainRenderer::render(
    const std::array<XrView, 2>& views,
    std::array<XrCompositionLayerProjectionView, 2>& projectionViews) {
    for (uint32_t eye = 0; eye < eyes_.size(); ++eye) {
        auto& swapchain = eyes_[eye];

        XrSwapchainImageAcquireInfo acquireInfo{XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
        uint32_t imageIndex = 0;
        checkXr(xrAcquireSwapchainImage(swapchain.handle, &acquireInfo, &imageIndex), "xrAcquireSwapchainImage");

        XrSwapchainImageWaitInfo waitInfo{XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};
        waitInfo.timeout = XR_INFINITE_DURATION;
        checkXr(xrWaitSwapchainImage(swapchain.handle, &waitInfo), "xrWaitSwapchainImage");

        clearImage(swapchain.images.at(imageIndex).image, swapchain.width, swapchain.height, eye);

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
}

void XrSwapchainRenderer::shutdown() noexcept {
    if (device_ != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(device_);
    }

    for (auto& eye : eyes_) {
        if (eye.handle != XR_NULL_HANDLE) {
            xrDestroySwapchain(eye.handle);
            eye.handle = XR_NULL_HANDLE;
        }
        eye.images.clear();
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
    session_ = XR_NULL_HANDLE;
}

} // namespace voxen::render
