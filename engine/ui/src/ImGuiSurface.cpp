#include <voxen/ui/ImGuiSurface.hpp>
#include <imgui.h>
#include <backends/imgui_impl_vulkan.h>
#include <stdexcept>
#include <string>

namespace voxen::ui {
namespace {
void check(VkResult result,const char* name) {
    if(result != VK_SUCCESS) throw std::runtime_error(std::string(name)+" VkResult="+std::to_string(result));
}
}
void ImGuiSurface::initialize(VkInstance instance,VkPhysicalDevice physical,VkDevice device,
                              VkQueue queue,uint32_t queueFamily) {
    physical_=physical; device_=device;
    constexpr uint32_t width=512, height=672;
    VkImageCreateInfo ci{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    ci.imageType=VK_IMAGE_TYPE_2D; ci.format=VK_FORMAT_R8G8B8A8_UNORM;
    ci.extent={width,height,1}; ci.mipLevels=1; ci.arrayLayers=1;
    ci.samples=VK_SAMPLE_COUNT_1_BIT; ci.tiling=VK_IMAGE_TILING_OPTIMAL;
    ci.usage=VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    check(vkCreateImage(device_,&ci,nullptr,&image_),"vkCreateImage(imgui)");
    VkMemoryRequirements memreq{}; vkGetImageMemoryRequirements(device_,image_,&memreq);
    VkPhysicalDeviceMemoryProperties props{}; vkGetPhysicalDeviceMemoryProperties(physical_,&props);
    uint32_t type=UINT32_MAX;
    for(uint32_t i=0;i<props.memoryTypeCount;i++)
        if((memreq.memoryTypeBits&(1u<<i)) &&
           (props.memoryTypes[i].propertyFlags&VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) {
            type=i;break;
        }
    if(type==UINT32_MAX) throw std::runtime_error("No device local memory for ImGui panel");
    VkMemoryAllocateInfo alloc{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    alloc.allocationSize=memreq.size; alloc.memoryTypeIndex=type;
    check(vkAllocateMemory(device_,&alloc,nullptr,&memory_),"vkAllocateMemory(imgui)");
    check(vkBindImageMemory(device_,image_,memory_,0),"vkBindImageMemory(imgui)");
    VkImageViewCreateInfo vi{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    vi.image=image_;vi.viewType=VK_IMAGE_VIEW_TYPE_2D;vi.format=ci.format;
    vi.subresourceRange.aspectMask=VK_IMAGE_ASPECT_COLOR_BIT;
    vi.subresourceRange.levelCount=1;vi.subresourceRange.layerCount=1;
    check(vkCreateImageView(device_,&vi,nullptr,&imageView_),"vkCreateImageView(imgui)");

    VkAttachmentDescription attachment{};
    attachment.format=ci.format;attachment.samples=VK_SAMPLE_COUNT_1_BIT;
    attachment.loadOp=VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachment.storeOp=VK_ATTACHMENT_STORE_OP_STORE;
    attachment.initialLayout=VK_IMAGE_LAYOUT_UNDEFINED;
    attachment.finalLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    VkAttachmentReference ref{0,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint=VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount=1;subpass.pColorAttachments=&ref;
    VkSubpassDependency dependency{};
    dependency.srcSubpass=0; dependency.dstSubpass=VK_SUBPASS_EXTERNAL;
    dependency.srcStageMask=VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstStageMask=VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependency.srcAccessMask=VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependency.dstAccessMask=VK_ACCESS_SHADER_READ_BIT;
    VkRenderPassCreateInfo rp{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    rp.attachmentCount=1;rp.pAttachments=&attachment;rp.subpassCount=1;
    rp.pSubpasses=&subpass;rp.dependencyCount=1;rp.pDependencies=&dependency;
    check(vkCreateRenderPass(device_,&rp,nullptr,&pass_),"vkCreateRenderPass(imgui)");
    VkFramebufferCreateInfo fb{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
    fb.renderPass=pass_;fb.attachmentCount=1;fb.pAttachments=&imageView_;
    fb.width=width;fb.height=height;fb.layers=1;
    check(vkCreateFramebuffer(device_,&fb,nullptr,&framebuffer_),"vkCreateFramebuffer(imgui)");

    VkSamplerCreateInfo sampler{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    sampler.magFilter=VK_FILTER_LINEAR;sampler.minFilter=VK_FILTER_LINEAR;
    sampler.addressModeU=sampler.addressModeV=sampler.addressModeW=VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler.maxLod=1.0f;
    check(vkCreateSampler(device_,&sampler,nullptr,&sampler_),"vkCreateSampler(imgui)");

    VkDescriptorPoolSize imguiSize{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,100};
    VkDescriptorPoolCreateInfo pool{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    pool.flags=VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool.maxSets=100;pool.poolSizeCount=1;pool.pPoolSizes=&imguiSize;
    check(vkCreateDescriptorPool(device_,&pool,nullptr,&imguiPool_),"vkCreateDescriptorPool(imgui)");
    VkDescriptorSetLayoutBinding binding{};
    binding.binding=0;binding.descriptorType=VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    binding.descriptorCount=1;binding.stageFlags=VK_SHADER_STAGE_FRAGMENT_BIT;
    VkDescriptorSetLayoutCreateInfo layout{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    layout.bindingCount=1;layout.pBindings=&binding;
    check(vkCreateDescriptorSetLayout(device_,&layout,nullptr,&panelLayout_),"vkCreateDescriptorSetLayout(panel)");
    pool.maxSets=1;
    check(vkCreateDescriptorPool(device_,&pool,nullptr,&panelPool_),"vkCreateDescriptorPool(panel)");
    VkDescriptorSetAllocateInfo set{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    set.descriptorPool=panelPool_;set.descriptorSetCount=1;set.pSetLayouts=&panelLayout_;
    check(vkAllocateDescriptorSets(device_,&set,&panelDescriptor_),"vkAllocateDescriptorSets(panel)");
    VkDescriptorImageInfo sampled{};
    sampled.sampler=sampler_;sampled.imageView=imageView_;
    sampled.imageLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    write.dstSet=panelDescriptor_;write.descriptorCount=1;
    write.descriptorType=VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;write.pImageInfo=&sampled;
    vkUpdateDescriptorSets(device_,1,&write,0,nullptr);

    ImGui_ImplVulkan_InitInfo info{};
    info.Instance=instance;info.PhysicalDevice=physical_;info.Device=device_;
    info.QueueFamily=queueFamily;info.Queue=queue;
    info.DescriptorPool=imguiPool_;info.MinImageCount=2;info.ImageCount=2;
    info.RenderPass=pass_;
    info.MSAASamples=VK_SAMPLE_COUNT_1_BIT;
    if(!ImGui_ImplVulkan_Init(&info)) throw std::runtime_error("ImGui Vulkan init failed");
    initialized_=true;
}
void ImGuiSurface::render(VkCommandBuffer cmd,ImDrawData* draw) {
    if(!initialized_)return;
    VkClearValue clear{};clear.color={{0.0f,0.014f,0.008f,0.04f}};
    VkRenderPassBeginInfo rp{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    rp.renderPass=pass_;rp.framebuffer=framebuffer_;
    rp.renderArea.extent={512,672};rp.clearValueCount=1;rp.pClearValues=&clear;
    vkCmdBeginRenderPass(cmd,&rp,VK_SUBPASS_CONTENTS_INLINE);
    ImGui_ImplVulkan_RenderDrawData(draw,cmd);
    vkCmdEndRenderPass(cmd);
}
void ImGuiSurface::shutdown() noexcept {
    if(device_==VK_NULL_HANDLE)return;
    if(initialized_) {ImGui_ImplVulkan_Shutdown();initialized_=false;}
    if(panelPool_)vkDestroyDescriptorPool(device_,panelPool_,nullptr);
    if(imguiPool_)vkDestroyDescriptorPool(device_,imguiPool_,nullptr);
    if(panelLayout_)vkDestroyDescriptorSetLayout(device_,panelLayout_,nullptr);
    if(sampler_)vkDestroySampler(device_,sampler_,nullptr);
    if(framebuffer_)vkDestroyFramebuffer(device_,framebuffer_,nullptr);
    if(pass_)vkDestroyRenderPass(device_,pass_,nullptr);
    if(imageView_)vkDestroyImageView(device_,imageView_,nullptr);
    if(image_)vkDestroyImage(device_,image_,nullptr);
    if(memory_)vkFreeMemory(device_,memory_,nullptr);
    device_=VK_NULL_HANDLE;
}
} // namespace voxen::ui
