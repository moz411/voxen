#include <voxen/render/XrSwapchainRenderer.hpp>
#ifdef VOXEN_WITH_GLB_IMPORT
#include <voxen/scene/GlbMeshDecoder.hpp>
#include <voxen/render/PanoramaImage.hpp>
#include <stdexcept>
#include <cstring>
#include <algorithm>
#include <limits>
#ifdef __ANDROID__
#include <android/log.h>
#endif

namespace voxen::render {
namespace {
struct LitVertex { float position[3]; float normal[3]; float uv[2]; };
void checked(VkResult result,const char* what) {
    if(result!=VK_SUCCESS)throw std::runtime_error(what);
}
void upload(VkDevice device,VkPhysicalDevice physicalDevice,const void* data,
            VkDeviceSize size,VkBufferUsageFlags usage,
            VkBuffer& buffer,VkDeviceMemory& memory) {
    if(size==0)throw std::runtime_error("GLB: zero GPU buffer");
    VkBufferCreateInfo create{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    create.size=size;create.usage=usage;create.sharingMode=VK_SHARING_MODE_EXCLUSIVE;
    checked(vkCreateBuffer(device,&create,nullptr,&buffer),"GLB vkCreateBuffer");
    VkMemoryRequirements req{};
    vkGetBufferMemoryRequirements(device,buffer,&req);
    VkPhysicalDeviceMemoryProperties properties{};
    vkGetPhysicalDeviceMemoryProperties(physicalDevice,&properties);
    uint32_t index=UINT32_MAX;
    for(uint32_t i=0;i<properties.memoryTypeCount;++i) {
        const auto flags=properties.memoryTypes[i].propertyFlags;
        constexpr VkMemoryPropertyFlags wanted=VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|
                                                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        if((req.memoryTypeBits&(1u<<i)) && (flags&wanted)==wanted){index=i;break;}
    }
    if(index==UINT32_MAX)throw std::runtime_error("GLB: no coherent upload memory");
    VkMemoryAllocateInfo alloc{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    alloc.allocationSize=req.size;alloc.memoryTypeIndex=index;
    checked(vkAllocateMemory(device,&alloc,nullptr,&memory),"GLB vkAllocateMemory");
    checked(vkBindBufferMemory(device,buffer,memory,0),"GLB vkBindBufferMemory");
    void* mapped=nullptr;
    checked(vkMapMemory(device,memory,0,size,0,&mapped),"GLB vkMapMemory");
    std::memcpy(mapped,data,static_cast<std::size_t>(size));
    vkUnmapMemory(device,memory);
}

uint32_t memoryType(VkPhysicalDevice gpu,uint32_t mask,VkMemoryPropertyFlags flags) {
    VkPhysicalDeviceMemoryProperties mem{};
    vkGetPhysicalDeviceMemoryProperties(gpu,&mem);
    for(uint32_t i=0;i<mem.memoryTypeCount;++i)
        if((mask&(1u<<i)) && (mem.memoryTypes[i].propertyFlags&flags)==flags)return i;
    throw std::runtime_error("GLB texture: Vulkan memory type unavailable");
}
void uploadTexture(VkDevice device,VkPhysicalDevice gpu,VkQueue queue,VkCommandPool pool,
                   VkDescriptorSetLayout layout,VkDescriptorPool descriptorPool,
                   const voxen::scene::gltf::EmbeddedImage& source,
                   VkImage& image,VkDeviceMemory& memory,VkImageView& view,
                   VkSampler& sampler,VkDescriptorSet& descriptor) {
    if(source.rgba.empty() || source.width<=0 || source.height<=0)return;
    const VkDeviceSize count=source.rgba.size();
    VkBuffer staging=VK_NULL_HANDLE;
    VkDeviceMemory stagingMemory=VK_NULL_HANDLE;
    VkCommandBuffer command=VK_NULL_HANDLE;
    try {
        upload(device,gpu,source.rgba.data(),count,VK_BUFFER_USAGE_TRANSFER_SRC_BIT,staging,stagingMemory);
        VkImageCreateInfo img{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
        img.imageType=VK_IMAGE_TYPE_2D;
        img.extent={static_cast<uint32_t>(source.width),static_cast<uint32_t>(source.height),1};
        img.mipLevels=1;img.arrayLayers=1;img.format=VK_FORMAT_R8G8B8A8_SRGB;
        img.tiling=VK_IMAGE_TILING_OPTIMAL;
        img.initialLayout=VK_IMAGE_LAYOUT_UNDEFINED;
        img.usage=VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_SAMPLED_BIT;
        img.samples=VK_SAMPLE_COUNT_1_BIT;img.sharingMode=VK_SHARING_MODE_EXCLUSIVE;
        checked(vkCreateImage(device,&img,nullptr,&image),"GLB vkCreateImage");
        VkMemoryRequirements req{};
        vkGetImageMemoryRequirements(device,image,&req);
        VkMemoryAllocateInfo alloc{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
        alloc.allocationSize=req.size;
        alloc.memoryTypeIndex=memoryType(gpu,req.memoryTypeBits,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        checked(vkAllocateMemory(device,&alloc,nullptr,&memory),"GLB texture allocation");
        checked(vkBindImageMemory(device,image,memory,0),"GLB bind image");
        VkCommandBufferAllocateInfo cb{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
        cb.commandPool=pool;cb.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;cb.commandBufferCount=1;
        checked(vkAllocateCommandBuffers(device,&cb,&command),"GLB allocate copy commands");
        VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        begin.flags=VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        checked(vkBeginCommandBuffer(command,&begin),"GLB begin copy");
        VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        barrier.srcQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
        barrier.image=image;
        barrier.subresourceRange.aspectMask=VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.levelCount=1;barrier.subresourceRange.layerCount=1;
        barrier.oldLayout=VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.dstAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;
        vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,nullptr,0,nullptr,1,&barrier);
        VkBufferImageCopy region{};
        region.imageSubresource.aspectMask=VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.layerCount=1;
        region.imageExtent=img.extent;
        vkCmdCopyBufferToImage(command,staging,image,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1,&region);
        barrier.oldLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask=VK_ACCESS_SHADER_READ_BIT;
        vkCmdPipelineBarrier(command,VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,0,0,nullptr,0,nullptr,1,&barrier);
        checked(vkEndCommandBuffer(command),"GLB end copy");
        VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
        submit.commandBufferCount=1;submit.pCommandBuffers=&command;
        checked(vkQueueSubmit(queue,1,&submit,VK_NULL_HANDLE),"GLB texture submit");
        checked(vkQueueWaitIdle(queue),"GLB texture completion");
        vkFreeCommandBuffers(device,pool,1,&command);command=VK_NULL_HANDLE;
        vkDestroyBuffer(device,staging,nullptr);staging=VK_NULL_HANDLE;
        vkFreeMemory(device,stagingMemory,nullptr);stagingMemory=VK_NULL_HANDLE;
        VkImageViewCreateInfo vi{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        vi.image=image;vi.viewType=VK_IMAGE_VIEW_TYPE_2D;vi.format=img.format;
        vi.subresourceRange.aspectMask=VK_IMAGE_ASPECT_COLOR_BIT;
        vi.subresourceRange.levelCount=1;vi.subresourceRange.layerCount=1;
        checked(vkCreateImageView(device,&vi,nullptr,&view),"GLB image view");
        VkSamplerCreateInfo si{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
        si.magFilter=VK_FILTER_LINEAR;si.minFilter=VK_FILTER_LINEAR;
        si.mipmapMode=VK_SAMPLER_MIPMAP_MODE_LINEAR;
        si.addressModeU=si.addressModeV=si.addressModeW=VK_SAMPLER_ADDRESS_MODE_REPEAT;
        si.maxLod=0;
        checked(vkCreateSampler(device,&si,nullptr,&sampler),"GLB sampler");
        VkDescriptorSetAllocateInfo ds{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
        ds.descriptorPool=descriptorPool;ds.descriptorSetCount=1;ds.pSetLayouts=&layout;
        checked(vkAllocateDescriptorSets(device,&ds,&descriptor),"GLB descriptor allocation");
        VkDescriptorImageInfo di{sampler,view,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
        VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        write.dstSet=descriptor;write.dstBinding=0;write.descriptorCount=1;
        write.descriptorType=VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write.pImageInfo=&di;
        vkUpdateDescriptorSets(device,1,&write,0,nullptr);
    } catch(...) {
        if(command)vkFreeCommandBuffers(device,pool,1,&command);
        if(staging)vkDestroyBuffer(device,staging,nullptr);
        if(stagingMemory)vkFreeMemory(device,stagingMemory,nullptr);
        throw;
    }
}
}
void XrSwapchainRenderer::releaseGlbGpu() noexcept {
    for(auto& p:glbPrimitives_) {
        if(p.vertexBuffer)vkDestroyBuffer(device_,p.vertexBuffer,nullptr);
        if(p.indexBuffer)vkDestroyBuffer(device_,p.indexBuffer,nullptr);
        if(p.vertexMemory)vkFreeMemory(device_,p.vertexMemory,nullptr);
        if(p.indexMemory)vkFreeMemory(device_,p.indexMemory,nullptr);
    }
    glbPrimitives_.clear();
    for(auto& tex:glbTextures_) {
        if(tex.sampler)vkDestroySampler(device_,tex.sampler,nullptr);
        if(tex.view)vkDestroyImageView(device_,tex.view,nullptr);
        if(tex.image)vkDestroyImage(device_,tex.image,nullptr);
        if(tex.memory)vkFreeMemory(device_,tex.memory,nullptr);
    }
    glbTextures_.clear();
    if(glbTextureDescriptorPool_) {
        vkDestroyDescriptorPool(device_,glbTextureDescriptorPool_,nullptr);
        glbTextureDescriptorPool_=VK_NULL_HANDLE;
    }
}
bool XrSwapchainRenderer::loadDemoGlb(const std::filesystem::path& file) {
    if(device_==VK_NULL_HANDLE)return false;
    std::vector<GlbGpuPrimitive> incoming;
    std::vector<GlbTexture> incomingTextures;
    VkDescriptorPool incomingPool=VK_NULL_HANDLE;
    try {
        const auto model=voxen::scene::gltf::loadMeshGlb(file);
        if(!model.images.empty() && glbTextureSetLayout_ && commandPool_) {
            VkDescriptorPoolSize size{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                     static_cast<uint32_t>(model.images.size())};
            VkDescriptorPoolCreateInfo ci{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
            ci.maxSets=size.descriptorCount;ci.poolSizeCount=1;ci.pPoolSizes=&size;
            checked(vkCreateDescriptorPool(device_,&ci,nullptr,&incomingPool),"GLB pool");
            incomingTextures.resize(model.images.size());
            for(size_t i=0;i<model.images.size();++i) {
                auto& tex=incomingTextures[i];
                uploadTexture(device_,physicalDevice_,queue_,commandPool_,glbTextureSetLayout_,
                              incomingPool,model.images[i],tex.image,tex.memory,
                              tex.view,tex.sampler,tex.descriptor);
            }
        }
        for(const auto& node:model.nodes) {
            for(auto primitiveId:node.primitives) {
                const auto& primitive=model.primitives.at(primitiveId);
                if(primitive.vertices.empty() || primitive.indices.empty())continue;
                std::vector<LitVertex> vertices;
                vertices.reserve(primitive.vertices.size());
                for(const auto& v:primitive.vertices)
                    vertices.push_back({{v.position[0],v.position[1],v.position[2]},
                                        {v.normal[0],v.normal[1],v.normal[2]},
                                        {v.uv[0],v.uv[1]}});
                GlbGpuPrimitive gpu{};
                gpu.worldMatrix=node.worldMatrix;
                gpu.baseColor=primitive.baseColor;
                gpu.textureIndex=primitive.baseColorTexture;
                incoming.push_back(gpu); // record immediately for exception-safe cleanup
                auto& target=incoming.back();
                upload(device_,physicalDevice_,vertices.data(),
                       static_cast<VkDeviceSize>(vertices.size()*sizeof(LitVertex)),
                       VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                       target.vertexBuffer,target.vertexMemory);
                upload(device_,physicalDevice_,primitive.indices.data(),
                       static_cast<VkDeviceSize>(primitive.indices.size()*sizeof(uint32_t)),
                       VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                       target.indexBuffer,target.indexMemory);
                target.indexCount=static_cast<uint32_t>(primitive.indices.size());
            }
        }
        // The current draw path is serialized on the same command buffer/queue.
        // Replacing resident geometry must wait until prior GPU use completes.
        checked(vkDeviceWaitIdle(device_),"GLB vkDeviceWaitIdle");
        releaseGlbGpu();
        glbPrimitives_=std::move(incoming);
        glbTextures_=std::move(incomingTextures);
        glbTextureDescriptorPool_=incomingPool;
        return !glbPrimitives_.empty();
    } catch(const std::exception&) {
        for(auto& tex:incomingTextures) {
            if(tex.sampler)vkDestroySampler(device_,tex.sampler,nullptr);
            if(tex.view)vkDestroyImageView(device_,tex.view,nullptr);
            if(tex.image)vkDestroyImage(device_,tex.image,nullptr);
            if(tex.memory)vkFreeMemory(device_,tex.memory,nullptr);
        }
        if(incomingPool)vkDestroyDescriptorPool(device_,incomingPool,nullptr);
        for(auto& part:incoming) {
            if(part.vertexBuffer)vkDestroyBuffer(device_,part.vertexBuffer,nullptr);
            if(part.indexBuffer)vkDestroyBuffer(device_,part.indexBuffer,nullptr);
            if(part.vertexMemory)vkFreeMemory(device_,part.vertexMemory,nullptr);
            if(part.indexMemory)vkFreeMemory(device_,part.indexMemory,nullptr);
        }
        return false;
    }
}

void XrSwapchainRenderer::releasePanoramaGpu() noexcept {
    if(!device_)return;
    if(panoramaTexture_.sampler)vkDestroySampler(device_,panoramaTexture_.sampler,nullptr);
    if(panoramaTexture_.view)vkDestroyImageView(device_,panoramaTexture_.view,nullptr);
    if(panoramaTexture_.image)vkDestroyImage(device_,panoramaTexture_.image,nullptr);
    if(panoramaTexture_.memory)vkFreeMemory(device_,panoramaTexture_.memory,nullptr);
    panoramaTexture_={};
    if(panoramaDescriptorPool_)vkDestroyDescriptorPool(device_,panoramaDescriptorPool_,nullptr);
    panoramaDescriptorPool_=VK_NULL_HANDLE;
}
bool XrSwapchainRenderer::loadPanoramaGpu(const std::filesystem::path& file) {
    if(!device_ || !panoramaSetLayout_ || !commandPool_)return false;
    GlbTexture incoming{};
    VkDescriptorPool pool=VK_NULL_HANDLE;
    try {
#ifdef __ANDROID__
        __android_log_print(ANDROID_LOG_INFO,"VoxenPanorama","Loading %s",file.filename().string().c_str());
#endif
        auto decoded=decodePanoramaImage(file);
#ifdef __ANDROID__
        __android_log_print(ANDROID_LOG_INFO,"VoxenPanorama","Decoded %dx%d",decoded.width,decoded.height);
#endif
        voxen::scene::gltf::EmbeddedImage image;
        image.width=decoded.width;
        image.height=decoded.height;
        image.rgba=std::move(decoded.rgba);
        VkDescriptorPoolSize size{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,1};
        VkDescriptorPoolCreateInfo info{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
        info.maxSets=1;
        info.poolSizeCount=1;
        info.pPoolSizes=&size;
        checked(vkCreateDescriptorPool(device_,&info,nullptr,&pool),"Panorama descriptor pool");
        uploadTexture(device_,physicalDevice_,queue_,commandPool_,panoramaSetLayout_,
                      pool,image,incoming.image,incoming.memory,incoming.view,
                      incoming.sampler,incoming.descriptor);
        checked(vkDeviceWaitIdle(device_),"Panorama replace GPU texture");
#ifdef __ANDROID__
        __android_log_print(ANDROID_LOG_INFO,"VoxenPanorama","GPU upload finished");
#endif
        releasePanoramaGpu();
        panoramaTexture_=incoming;
        panoramaDescriptorPool_=pool;
        return true;
    } catch(const std::exception& e) {
#ifdef __ANDROID__
        __android_log_print(ANDROID_LOG_ERROR,"VoxenPanorama","Load failed: %s",e.what());
#endif
        if(incoming.sampler)vkDestroySampler(device_,incoming.sampler,nullptr);
        if(incoming.view)vkDestroyImageView(device_,incoming.view,nullptr);
        if(incoming.image)vkDestroyImage(device_,incoming.image,nullptr);
        if(incoming.memory)vkFreeMemory(device_,incoming.memory,nullptr);
        if(pool)vkDestroyDescriptorPool(device_,pool,nullptr);
        return false;
    }
}
} // namespace voxen::render
#endif
