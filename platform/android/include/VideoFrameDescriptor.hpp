#pragma once
#ifdef __ANDROID__
#include "AndroidVideoImage.hpp"
#include <vulkan/vulkan.h>

namespace voxen::video {

// One imported frame's immutable-YCbCr-sampler descriptor.
// Frame/VideoImage must outlive this binding and GPU submissions using it.
// No image layout transition or external producer synchronization is implied.
class VideoFrameDescriptor final {
public:
    ~VideoFrameDescriptor() { reset(); }
    VideoFrameDescriptor() = default;
    VideoFrameDescriptor(const VideoFrameDescriptor&) = delete;
    VideoFrameDescriptor& operator=(const VideoFrameDescriptor&) = delete;

    bool create(VkDevice device, const AndroidVideoImage& image,
                uint32_t combinedSamplerDescriptorCount = 4);
    void reset() noexcept;
    VkDescriptorSet set() const noexcept { return set_; }
    VkDescriptorSetLayout layout() const noexcept { return layout_; }
private:
    VkDevice device_ = VK_NULL_HANDLE;
    VkDescriptorSetLayout layout_ = VK_NULL_HANDLE;
    VkDescriptorPool pool_ = VK_NULL_HANDLE;
    VkDescriptorSet set_ = VK_NULL_HANDLE;
};
}
#endif
