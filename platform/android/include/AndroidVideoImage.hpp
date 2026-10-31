#pragma once
#ifdef __ANDROID__
#include <android/hardware_buffer.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_android.h>

namespace voxen::video {

// Owned imported image. Caller retains the original HardwareBuffer/Image until
// GPU work using this image is complete. No implicit queue synchronization.
class AndroidVideoImage final {
public:
    AndroidVideoImage() = default;
    ~AndroidVideoImage() { reset(); }
    AndroidVideoImage(const AndroidVideoImage&) = delete;
    AndroidVideoImage& operator=(const AndroidVideoImage&) = delete;

    // Device must have VK_ANDROID_external_memory_android_hardware_buffer and
    // samplerYcbcrConversion enabled. Returns false without leaking resources.
    bool importBuffer(VkPhysicalDevice physicalDevice, VkDevice device, AHardwareBuffer* buffer);
    // Records an acquire transition before sampling. Caller must arrange
    // producer readiness and submit/wait for completion separately.
    void recordAcquireForSampling(VkCommandBuffer commandBuffer) const noexcept;
    void reset() noexcept;
    VkImage image() const noexcept { return image_; }
    VkImageView view() const noexcept { return view_; }
    VkSampler sampler() const noexcept { return sampler_; }
    // The YCbCr sampler must be immutable in the descriptor-set layout.
    VkSamplerYcbcrConversion conversion() const noexcept { return conversion_; }
    uint32_t combinedSamplerDescriptorCount() const noexcept { return combinedSamplerDescriptorCount_; }
    bool valid() const noexcept { return image_ != VK_NULL_HANDLE && view_ != VK_NULL_HANDLE; }

private:
    uint32_t combinedSamplerDescriptorCount_ = 0;
    VkDevice device_ = VK_NULL_HANDLE;
    VkImage image_ = VK_NULL_HANDLE;
    VkDeviceMemory memory_ = VK_NULL_HANDLE;
    VkImageView view_ = VK_NULL_HANDLE;
    VkSampler sampler_ = VK_NULL_HANDLE;
    VkSamplerYcbcrConversion conversion_ = VK_NULL_HANDLE;
};
}
#endif
