#include "AndroidVideoImage.hpp"
#ifdef __ANDROID__
#include <android/log.h>
#include <cstdint>
#include <vulkan/vulkan_android.h>

namespace voxen::video {
namespace {
uint32_t chooseMemoryType(VkPhysicalDevice physicalDevice, uint32_t bits) {
    VkPhysicalDeviceMemoryProperties properties{};
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &properties);
    for (uint32_t i = 0; i < properties.memoryTypeCount; ++i) {
        if ((bits & (1u << i)) &&
            (properties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
            return i;
    }
    for (uint32_t i = 0; i < properties.memoryTypeCount; ++i)
        if (bits & (1u << i)) return i;
    return UINT32_MAX;
}
}
bool AndroidVideoImage::importBuffer(VkPhysicalDevice physicalDevice, VkDevice device,
                               AHardwareBuffer* buffer) {
    reset();
    if (!physicalDevice || !device || !buffer) return false;
    auto getProperties = reinterpret_cast<PFN_vkGetAndroidHardwareBufferPropertiesANDROID>(
        vkGetDeviceProcAddr(device, "vkGetAndroidHardwareBufferPropertiesANDROID"));
    auto createConversion = reinterpret_cast<PFN_vkCreateSamplerYcbcrConversion>(
        vkGetDeviceProcAddr(device, "vkCreateSamplerYcbcrConversion"));
    auto destroyConversion = reinterpret_cast<PFN_vkDestroySamplerYcbcrConversion>(
        vkGetDeviceProcAddr(device, "vkDestroySamplerYcbcrConversion"));
    if (!getProperties || !createConversion || !destroyConversion) return false;

    VkAndroidHardwareBufferFormatPropertiesANDROID format{
        VK_STRUCTURE_TYPE_ANDROID_HARDWARE_BUFFER_FORMAT_PROPERTIES_ANDROID};
    VkAndroidHardwareBufferPropertiesANDROID properties{
        VK_STRUCTURE_TYPE_ANDROID_HARDWARE_BUFFER_PROPERTIES_ANDROID};
    properties.pNext = &format;
    if (getProperties(device, buffer, &properties) != VK_SUCCESS ||
        !(format.formatFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT)) return false;

    AHardwareBuffer_Desc desc{};
    AHardwareBuffer_describe(buffer, &desc);
    if (!desc.width || !desc.height || !properties.memoryTypeBits) return false;
    const bool external = format.format == VK_FORMAT_UNDEFINED;
    if (external && !format.externalFormat) return false;

    // Query the true descriptor multiplicity instead of guessing the count
    // for a multi-plane external YCbCr combined image sampler.
    VkPhysicalDeviceExternalImageFormatInfo externalImage{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_IMAGE_FORMAT_INFO};
    externalImage.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_ANDROID_HARDWARE_BUFFER_BIT_ANDROID;
    VkPhysicalDeviceExternalFormatInfoANDROID externalInfo{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_FORMAT_INFO_ANDROID};
    externalInfo.externalFormat = format.externalFormat;
    externalImage.pNext = external ? &externalInfo : nullptr;
    VkPhysicalDeviceImageFormatInfo2 formatInfo{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_FORMAT_INFO_2};
    formatInfo.pNext = &externalImage;
    formatInfo.format = format.format;
    formatInfo.type = VK_IMAGE_TYPE_2D;
    formatInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    formatInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
    VkSamplerYcbcrConversionImageFormatProperties conversionProperties{
        VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_IMAGE_FORMAT_PROPERTIES};
    VkImageFormatProperties2 imageProperties{
        VK_STRUCTURE_TYPE_IMAGE_FORMAT_PROPERTIES_2};
    imageProperties.pNext = &conversionProperties;
    if (vkGetPhysicalDeviceImageFormatProperties2(
            physicalDevice, &formatInfo, &imageProperties) != VK_SUCCESS ||
        !conversionProperties.combinedImageSamplerDescriptorCount) return false;
    combinedSamplerDescriptorCount_ =
        conversionProperties.combinedImageSamplerDescriptorCount;
    const uint32_t type = chooseMemoryType(physicalDevice, properties.memoryTypeBits);
    if (type == UINT32_MAX) return false;
    device_ = device;

    VkExternalFormatANDROID externalFormat{VK_STRUCTURE_TYPE_EXTERNAL_FORMAT_ANDROID};
    externalFormat.externalFormat = format.externalFormat;
    VkExternalMemoryImageCreateInfo externalMemory{
        VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO};
    externalMemory.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_ANDROID_HARDWARE_BUFFER_BIT_ANDROID;
    if (external) externalMemory.pNext = &externalFormat;

    VkImageCreateInfo imageInfo{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    imageInfo.pNext = &externalMemory;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = format.format;
    imageInfo.extent = {desc.width, desc.height, 1};
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    if (vkCreateImage(device_, &imageInfo, nullptr, &image_) != VK_SUCCESS) {
        reset();
        return false;
    }

    VkImportAndroidHardwareBufferInfoANDROID importInfo{
        VK_STRUCTURE_TYPE_IMPORT_ANDROID_HARDWARE_BUFFER_INFO_ANDROID};
    importInfo.buffer = buffer;
    VkMemoryDedicatedAllocateInfo dedicated{
        VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO};
    dedicated.image = image_;
    dedicated.pNext = &importInfo;
    VkMemoryAllocateInfo allocation{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    allocation.pNext = &dedicated;
    allocation.allocationSize = properties.allocationSize;
    allocation.memoryTypeIndex = type;
    if (vkAllocateMemory(device_, &allocation, nullptr, &memory_) != VK_SUCCESS ||
        vkBindImageMemory(device_, image_, memory_, 0) != VK_SUCCESS) {
        reset();
        return false;
    }

    VkSamplerYcbcrConversionCreateInfo conversionInfo{
        VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_CREATE_INFO};
    conversionInfo.format = format.format;
    conversionInfo.ycbcrModel = format.suggestedYcbcrModel;
    conversionInfo.ycbcrRange = format.suggestedYcbcrRange;
    conversionInfo.components = format.samplerYcbcrConversionComponents;
    conversionInfo.xChromaOffset = format.suggestedXChromaOffset;
    conversionInfo.yChromaOffset = format.suggestedYChromaOffset;
    conversionInfo.chromaFilter = VK_FILTER_NEAREST;
    conversionInfo.forceExplicitReconstruction = VK_FALSE;
    if (external) conversionInfo.pNext = &externalFormat;
    if (createConversion(device_, &conversionInfo, nullptr, &conversion_) != VK_SUCCESS) {
        reset();
        return false;
    }

    VkSamplerYcbcrConversionInfo conversionChain{
        VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_INFO};
    conversionChain.conversion = conversion_;
    VkImageViewCreateInfo viewInfo{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    viewInfo.pNext = &conversionChain;
    viewInfo.image = image_;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format.format;
    viewInfo.components = {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
                           VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY};
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.layerCount = 1;
    if (vkCreateImageView(device_, &viewInfo, nullptr, &view_) != VK_SUCCESS) {
        reset();
        return false;
    }

    VkSamplerCreateInfo samplerInfo{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    samplerInfo.pNext = &conversionChain;
    samplerInfo.magFilter = VK_FILTER_NEAREST;
    samplerInfo.minFilter = VK_FILTER_NEAREST;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.maxLod = 0.0f;
    if (vkCreateSampler(device_, &samplerInfo, nullptr, &sampler_) != VK_SUCCESS) {
        reset();
        return false;
    }
    __android_log_print(ANDROID_LOG_INFO, "VoxenVideo",
        "Imported video VkImage %ux%u external=%d", desc.width, desc.height, external);
    return true;
}

void AndroidVideoImage::recordAcquireForSampling(
    VkCommandBuffer commandBuffer) const noexcept {
    if (!commandBuffer || !image_) return;
    // This is only the image layout/access barrier. The caller must first
    // synchronize the MediaCodec producer (native acquire fence), and if
    // necessary perform foreign queue-family ownership acquisition.
    // GENERAL is the externally maintained layout assumed by this contract;
    // do not substitute UNDEFINED, which discards the decoded contents.
    VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image_;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.layerCount = 1;
    vkCmdPipelineBarrier(commandBuffer,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
        0, nullptr, 0, nullptr, 1, &barrier);
}

void AndroidVideoImage::reset() noexcept {
    if (device_ != VK_NULL_HANDLE) {
        if (sampler_) vkDestroySampler(device_, sampler_, nullptr);
        if (view_) vkDestroyImageView(device_, view_, nullptr);
        if (conversion_) {
            auto fn = reinterpret_cast<PFN_vkDestroySamplerYcbcrConversion>(
                vkGetDeviceProcAddr(device_, "vkDestroySamplerYcbcrConversion"));
            if (fn) fn(device_, conversion_, nullptr);
        }
        if (image_) vkDestroyImage(device_, image_, nullptr);
        if (memory_) vkFreeMemory(device_, memory_, nullptr);
    }
    sampler_ = VK_NULL_HANDLE;
    view_ = VK_NULL_HANDLE;
    conversion_ = VK_NULL_HANDLE;
    image_ = VK_NULL_HANDLE;
    memory_ = VK_NULL_HANDLE;
    device_ = VK_NULL_HANDLE;
    combinedSamplerDescriptorCount_ = 0;
}
} // namespace voxen::video
#endif
