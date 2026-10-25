#include <voxen/render/XrSwapchainRenderer.hpp>
#ifdef VOXEN_WITH_GLB_IMPORT
#include <voxen/scene/GlbMeshDecoder.hpp>
#include <stdexcept>
#include <cstring>
#include <algorithm>
#include <limits>

namespace voxen::render {
namespace {
struct LitVertex { float position[3]; float normal[3]; };
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
}
void XrSwapchainRenderer::releaseGlbGpu() noexcept {
    for(auto& p:glbPrimitives_) {
        if(p.vertexBuffer)vkDestroyBuffer(device_,p.vertexBuffer,nullptr);
        if(p.indexBuffer)vkDestroyBuffer(device_,p.indexBuffer,nullptr);
        if(p.vertexMemory)vkFreeMemory(device_,p.vertexMemory,nullptr);
        if(p.indexMemory)vkFreeMemory(device_,p.indexMemory,nullptr);
    }
    glbPrimitives_.clear();
}
bool XrSwapchainRenderer::loadDemoGlb(const std::filesystem::path& file) {
    if(device_==VK_NULL_HANDLE)return false;
    std::vector<GlbGpuPrimitive> incoming;
    try {
        const auto model=voxen::scene::gltf::loadMeshGlb(file);
        for(const auto& node:model.nodes) {
            for(auto primitiveId:node.primitives) {
                const auto& primitive=model.primitives.at(primitiveId);
                if(primitive.vertices.empty() || primitive.indices.empty())continue;
                std::vector<LitVertex> vertices;
                vertices.reserve(primitive.vertices.size());
                for(const auto& v:primitive.vertices)
                    vertices.push_back({{v.position[0],v.position[1],v.position[2]},
                                        {v.normal[0],v.normal[1],v.normal[2]}});
                GlbGpuPrimitive gpu{};
                gpu.worldMatrix=node.worldMatrix;
                gpu.baseColor=primitive.baseColor;
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
        return !glbPrimitives_.empty();
    } catch(const std::exception&) {
        for(auto& part:incoming) {
            if(part.vertexBuffer)vkDestroyBuffer(device_,part.vertexBuffer,nullptr);
            if(part.indexBuffer)vkDestroyBuffer(device_,part.indexBuffer,nullptr);
            if(part.vertexMemory)vkFreeMemory(device_,part.vertexMemory,nullptr);
            if(part.indexMemory)vkFreeMemory(device_,part.indexMemory,nullptr);
        }
        return false;
    }
}
} // namespace voxen::render
#endif
