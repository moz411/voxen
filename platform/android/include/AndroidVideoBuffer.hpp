#pragma once
#ifdef __ANDROID__
#include <android/hardware_buffer.h>
#include <vulkan/vulkan.h>

namespace voxen::video {

// Hardware capabilities / format preflight for MediaCodec-backed frames.
// Does NOT create a VkImage: the importer must use the reported externalFormat
// and allocationSize with the appropriate YCbCr sampler and synchronization.
struct AndroidVideoBufferInfo {
    bool supported = false;
    uint32_t width = 0;
    uint32_t height = 0;
    VkFormat format = VK_FORMAT_UNDEFINED;
    uint64_t externalFormat = 0;
    VkDeviceSize allocationSize = 0;
    uint32_t memoryTypeBits = 0;
    VkFormatFeatureFlags formatFeatures = 0;
};

AndroidVideoBufferInfo inspectVideoBuffer(
    VkDevice device, AHardwareBuffer* buffer) noexcept;

} // namespace voxen::video
#endif
