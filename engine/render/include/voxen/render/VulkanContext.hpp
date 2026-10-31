#pragma once

#include <cstdint>

#include <openxr/openxr.h>
#include <vulkan/vulkan.h>

namespace voxen::render {

class VulkanContext final {
public:
    VulkanContext() = default;
    ~VulkanContext();

    VulkanContext(const VulkanContext&) = delete;
    VulkanContext& operator=(const VulkanContext&) = delete;

    void initialize(XrInstance xrInstance, XrSystemId systemId);
    void shutdown() noexcept;

    [[nodiscard]] VkInstance instance() const noexcept { return instance_; }
    [[nodiscard]] VkPhysicalDevice physicalDevice() const noexcept { return physicalDevice_; }
    [[nodiscard]] VkDevice device() const noexcept { return device_; }
    [[nodiscard]] VkQueue graphicsQueue() const noexcept { return graphicsQueue_; }
    [[nodiscard]] uint32_t graphicsQueueFamily() const noexcept { return graphicsQueueFamily_; }

private:
    VkInstance instance_ = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;
    VkDevice device_ = VK_NULL_HANDLE;
    VkQueue graphicsQueue_ = VK_NULL_HANDLE;
    uint32_t graphicsQueueFamily_ = 0;
};

} // namespace voxen::render
