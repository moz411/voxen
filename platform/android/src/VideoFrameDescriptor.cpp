#include "VideoFrameDescriptor.hpp"
#ifdef __ANDROID__
namespace voxen::video {
bool VideoFrameDescriptor::create(VkDevice device, const AndroidVideoImage& image,
                                  uint32_t combinedSamplerDescriptorCount) {
    reset();
    if (!device || !image.valid() || !image.sampler() ||
        combinedSamplerDescriptorCount == 0) return false;
    device_ = device;
    const VkSampler immutableSampler = image.sampler();
    VkDescriptorSetLayoutBinding binding{};
    binding.binding = 0;
    binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    binding.descriptorCount = 1;
    binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    binding.pImmutableSamplers = &immutableSampler;
    VkDescriptorSetLayoutCreateInfo layoutInfo{
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &binding;
    if (vkCreateDescriptorSetLayout(device_, &layoutInfo, nullptr, &layout_) != VK_SUCCESS) {
        reset();
        return false;
    }
    // Allocates precisely the combined-sampler descriptor count queried
    // from VkSamplerYcbcrConversionImageFormatProperties.
    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSize.descriptorCount = combinedSamplerDescriptorCount;
    VkDescriptorPoolCreateInfo poolInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    poolInfo.maxSets = 1;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    if (vkCreateDescriptorPool(device_, &poolInfo, nullptr, &pool_) != VK_SUCCESS) {
        reset();
        return false;
    }
    VkDescriptorSetAllocateInfo allocation{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    allocation.descriptorPool = pool_;
    allocation.descriptorSetCount = 1;
    allocation.pSetLayouts = &layout_;
    if (vkAllocateDescriptorSets(device_, &allocation, &set_) != VK_SUCCESS) {
        reset();
        return false;
    }
    VkDescriptorImageInfo descriptor{};
    descriptor.imageView = image.view();
    descriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    // sampler is immutable, so descriptor.sampler is intentionally null.
    VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    write.dstSet = set_;
    write.dstBinding = 0;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.pImageInfo = &descriptor;
    vkUpdateDescriptorSets(device_, 1, &write, 0, nullptr);
    return true;
}
void VideoFrameDescriptor::reset() noexcept {
    if (device_) {
        if (pool_) vkDestroyDescriptorPool(device_, pool_, nullptr);
        if (layout_) vkDestroyDescriptorSetLayout(device_, layout_, nullptr);
    }
    pool_ = VK_NULL_HANDLE;
    layout_ = VK_NULL_HANDLE;
    set_ = VK_NULL_HANDLE;
    device_ = VK_NULL_HANDLE;
}
}
#endif
