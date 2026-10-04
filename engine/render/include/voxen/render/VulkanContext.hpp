#pragma once

namespace voxen::render {

// Owns Vulkan instance/device resources required by the OpenXR runtime.
class VulkanContext final {
public:
    VulkanContext() = default;
    ~VulkanContext() = default;

    VulkanContext(const VulkanContext&) = delete;
    VulkanContext& operator=(const VulkanContext&) = delete;
};

} // namespace voxen::render
