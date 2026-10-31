#include "AndroidVideoBuffer.hpp"
#ifdef __ANDROID__
#include <android/log.h>
#include <vulkan/vulkan_android.h>

namespace voxen::video {

AndroidVideoBufferInfo inspectVideoBuffer(
    VkDevice device, AHardwareBuffer* buffer) noexcept {
    AndroidVideoBufferInfo result{};
    if (device == VK_NULL_HANDLE || buffer == nullptr) return result;

    AHardwareBuffer_Desc desc{};
    AHardwareBuffer_describe(buffer, &desc);
    result.width = desc.width;
    result.height = desc.height;

    const auto propertiesFn =
        reinterpret_cast<PFN_vkGetAndroidHardwareBufferPropertiesANDROID>(
            vkGetDeviceProcAddr(device, "vkGetAndroidHardwareBufferPropertiesANDROID"));
    if (!propertiesFn) {
        __android_log_print(ANDROID_LOG_WARN, "VoxenVideo",
            "vkGetAndroidHardwareBufferPropertiesANDROID unavailable");
        return result;
    }

    VkAndroidHardwareBufferFormatPropertiesANDROID format{
        VK_STRUCTURE_TYPE_ANDROID_HARDWARE_BUFFER_FORMAT_PROPERTIES_ANDROID};
    VkAndroidHardwareBufferPropertiesANDROID properties{
        VK_STRUCTURE_TYPE_ANDROID_HARDWARE_BUFFER_PROPERTIES_ANDROID};
    properties.pNext = &format;

    const VkResult status = propertiesFn(device, buffer, &properties);
    if (status != VK_SUCCESS) {
        __android_log_print(ANDROID_LOG_WARN, "VoxenVideo",
            "AHardwareBuffer format query failed: VkResult=%d", status);
        return result;
    }

    result.format = format.format;
    result.externalFormat = format.externalFormat;
    result.allocationSize = properties.allocationSize;
    result.memoryTypeBits = properties.memoryTypeBits;
    result.formatFeatures = format.formatFeatures;
    result.supported = result.memoryTypeBits != 0 &&
        (result.format != VK_FORMAT_UNDEFINED || result.externalFormat != 0) &&
        (result.formatFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) != 0;

    __android_log_print(ANDROID_LOG_INFO, "VoxenVideo",
        "AHB %ux%u vkFormat=%d externalFormat=%llu memTypes=0x%x sampleable=%d",
        result.width, result.height, static_cast<int>(result.format),
        static_cast<unsigned long long>(result.externalFormat),
        result.memoryTypeBits, result.supported ? 1 : 0);
    return result;
}

} // namespace voxen::video
#endif
