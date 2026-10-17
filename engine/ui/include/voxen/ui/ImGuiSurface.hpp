#pragma once
#include <vulkan/vulkan.h>
struct ImDrawData;

namespace voxen::ui {
class ImGuiSurface {
public:
    void initialize(VkInstance instance, VkPhysicalDevice physical, VkDevice device,
                    VkQueue queue, uint32_t queueFamily);
    void render(VkCommandBuffer command, ImDrawData* draw);
    void shutdown() noexcept;
    VkDescriptorSet descriptor() const noexcept { return panelDescriptor_; }
    VkDescriptorSetLayout descriptorLayout() const noexcept { return panelLayout_; }
    bool ready() const noexcept { return initialized_; }
private:
    VkPhysicalDevice physical_ = VK_NULL_HANDLE;
    VkDevice device_ = VK_NULL_HANDLE;
    VkImage image_ = VK_NULL_HANDLE;
    VkDeviceMemory memory_ = VK_NULL_HANDLE;
    VkImageView imageView_ = VK_NULL_HANDLE;
    VkSampler sampler_ = VK_NULL_HANDLE;
    VkRenderPass pass_ = VK_NULL_HANDLE;
    VkFramebuffer framebuffer_ = VK_NULL_HANDLE;
    VkDescriptorPool imguiPool_ = VK_NULL_HANDLE;
    VkDescriptorPool panelPool_ = VK_NULL_HANDLE;
    VkDescriptorSetLayout panelLayout_ = VK_NULL_HANDLE;
    VkDescriptorSet panelDescriptor_ = VK_NULL_HANDLE;
    bool initialized_ = false;
};
} // namespace voxen::ui
