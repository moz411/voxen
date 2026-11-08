#pragma once
// Packed, GPU-ready Open Brush geometry. All strokes share one vertex/index
// allocation; draw ranges retain their original brush GUID for future materials.
#include <voxen/render/OpenBrushCompiledScene.hpp>
#include <vulkan/vulkan.h>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace voxen::render::openbrush {
struct MeshVertex {
    float position[3], normal[3], color[4], uv[2];
};
static_assert(sizeof(MeshVertex)==48);
struct MeshDraw {
    std::string brushGuid;
    std::uint32_t firstIndex=0, indexCount=0;
};
struct PackedMeshes {
    std::vector<MeshVertex> vertices;
    std::vector<std::uint32_t> indices;
    std::vector<MeshDraw> draws;
};
inline PackedMeshes packMeshes(const CompiledScene& scene) {
    PackedMeshes out;
    for(const auto& mesh:scene.meshes) {
        const std::size_t n=mesh.positions.size()/3;
        if(mesh.positions.size()!=n*3 || mesh.normals.size()!=n*3 ||
           mesh.colors.size()!=n*4 || mesh.uv0.size()!=n*2 ||
           out.vertices.size()+n>std::numeric_limits<std::uint32_t>::max() ||
           out.indices.size()+mesh.indices.size()>std::numeric_limits<std::uint32_t>::max())
            throw std::runtime_error("VXMSH: invalid mesh attributes or size");
        const auto base=static_cast<std::uint32_t>(out.vertices.size());
        for(std::size_t i=0;i<n;i++) {
            MeshVertex v{};
            std::memcpy(v.position,mesh.positions.data()+i*3,3*sizeof(float));
            std::memcpy(v.normal,mesh.normals.data()+i*3,3*sizeof(float));
            std::memcpy(v.color,mesh.colors.data()+i*4,4*sizeof(float));
            std::memcpy(v.uv,mesh.uv0.data()+i*2,2*sizeof(float));
            out.vertices.push_back(v);
        }
        MeshDraw draw{mesh.brushGuid,static_cast<std::uint32_t>(out.indices.size()),
                      static_cast<std::uint32_t>(mesh.indices.size())};
        for(auto index:mesh.indices) {
            if(index>=n)throw std::runtime_error("VXMSH: index out of range");
            out.indices.push_back(base+index);
        }
        out.draws.push_back(std::move(draw));
    }
    return out;
}
inline VkVertexInputBindingDescription meshBinding() {
    return {0,sizeof(MeshVertex),VK_VERTEX_INPUT_RATE_VERTEX};
}
inline std::array<VkVertexInputAttributeDescription,4> meshAttributes() {
    return {{{0,0,VK_FORMAT_R32G32B32_SFLOAT,0},
             {1,0,VK_FORMAT_R32G32B32_SFLOAT,12},
             {2,0,VK_FORMAT_R32G32B32A32_SFLOAT,24},
             {3,0,VK_FORMAT_R32G32_SFLOAT,40}}};
}
class CompiledMeshGpu {
public:
    CompiledMeshGpu()=default;
    CompiledMeshGpu(const CompiledMeshGpu&)=delete;
    CompiledMeshGpu& operator=(const CompiledMeshGpu&)=delete;
    ~CompiledMeshGpu()=default; // Explicit destroy(device), after GPU idle.
    bool empty() const {return draws_.empty();}
    std::size_t drawCount() const {return draws_.size();}
    void upload(VkDevice device,VkPhysicalDevice physical,const PackedMeshes& data) {
        if(vertex_||index_)throw std::logic_error("VXMSH: destroy before upload");
        if(data.vertices.empty()||data.indices.empty())return;
        try {
            makeBuffer(device,physical,data.vertices.data(),data.vertices.size()*sizeof(MeshVertex),
                       VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,vertex_,vertexMemory_);
            makeBuffer(device,physical,data.indices.data(),data.indices.size()*sizeof(std::uint32_t),
                       VK_BUFFER_USAGE_INDEX_BUFFER_BIT,index_,indexMemory_);
            draws_=data.draws;
        } catch(...) {destroy(device);throw;}
    }
    void record(VkCommandBuffer command,VkPipeline pipeline,VkPipelineLayout layout,
                const std::array<float,16>& viewProjection) const {
        if(empty()||!pipeline)return;
        vkCmdBindPipeline(command,VK_PIPELINE_BIND_POINT_GRAPHICS,pipeline);
        VkDeviceSize offset=0;
        vkCmdBindVertexBuffers(command,0,1,&vertex_,&offset);
        vkCmdBindIndexBuffer(command,index_,0,VK_INDEX_TYPE_UINT32);
        vkCmdPushConstants(command,layout,VK_SHADER_STAGE_VERTEX_BIT,0,64,viewProjection.data());
        for(const auto& draw:draws_)
            if(draw.indexCount)vkCmdDrawIndexed(command,draw.indexCount,1,draw.firstIndex,0,0);
    }
    void destroy(VkDevice device) {
        if(vertex_)vkDestroyBuffer(device,vertex_,nullptr);
        if(index_)vkDestroyBuffer(device,index_,nullptr);
        if(vertexMemory_)vkFreeMemory(device,vertexMemory_,nullptr);
        if(indexMemory_)vkFreeMemory(device,indexMemory_,nullptr);
        vertex_=index_=VK_NULL_HANDLE;
        vertexMemory_=indexMemory_=VK_NULL_HANDLE;
        draws_.clear();
    }
private:
    static void makeBuffer(VkDevice device,VkPhysicalDevice physical,const void* src,
                           std::size_t bytes,VkBufferUsageFlags usage,
                           VkBuffer& buffer,VkDeviceMemory& memory) {
        VkBufferCreateInfo info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        info.size=bytes;info.usage=usage;info.sharingMode=VK_SHARING_MODE_EXCLUSIVE;
        if(vkCreateBuffer(device,&info,nullptr,&buffer)!=VK_SUCCESS)
            throw std::runtime_error("VXMSH: vkCreateBuffer failed");
        VkMemoryRequirements req{};
        vkGetBufferMemoryRequirements(device,buffer,&req);
        VkPhysicalDeviceMemoryProperties props{};
        vkGetPhysicalDeviceMemoryProperties(physical,&props);
        std::uint32_t type=UINT32_MAX;
        constexpr auto wanted=VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        for(std::uint32_t i=0;i<props.memoryTypeCount;i++)
            if((req.memoryTypeBits&(1u<<i))&&(props.memoryTypes[i].propertyFlags&wanted)==wanted)
                {type=i;break;}
        if(type==UINT32_MAX)throw std::runtime_error("VXMSH: no host-coherent memory");
        VkMemoryAllocateInfo alloc{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
        alloc.allocationSize=req.size;alloc.memoryTypeIndex=type;
        if(vkAllocateMemory(device,&alloc,nullptr,&memory)!=VK_SUCCESS)
            throw std::runtime_error("VXMSH: vkAllocateMemory failed");
        if(vkBindBufferMemory(device,buffer,memory,0)!=VK_SUCCESS)
            throw std::runtime_error("VXMSH: vkBindBufferMemory failed");
        void* mapped=nullptr;
        if(vkMapMemory(device,memory,0,bytes,0,&mapped)!=VK_SUCCESS)
            throw std::runtime_error("VXMSH: vkMapMemory failed");
        std::memcpy(mapped,src,bytes);
        vkUnmapMemory(device,memory);
    }
    VkBuffer vertex_{VK_NULL_HANDLE},index_{VK_NULL_HANDLE};
    VkDeviceMemory vertexMemory_{VK_NULL_HANDLE},indexMemory_{VK_NULL_HANDLE};
    std::vector<MeshDraw> draws_;
};
} // namespace voxen::render::openbrush
