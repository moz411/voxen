#pragma once
// Host-visible, coherent Vulkan instance buffer for Open Brush Dots.
// Caller owns synchronization: upload only while the buffer is not in flight.
// Usage: dots.upload(device, physicalDevice, decodedInstances);
//        VkBuffer buffer = dots.buffer(); uint32_t count = dots.count();
//        dots.destroy(device) before destroying the VkDevice.
#include <voxen/render/OpenBrushDots.hpp>
#include <vulkan/vulkan.h>
#include <cstdint>
#include <cstddef>
#include <array>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <vector>

namespace voxen::render::openbrush {
static_assert(sizeof(DotInstance) == 32, "Dots GPU instance stride mismatch");
static_assert(offsetof(DotInstance, center) == 0);
static_assert(offsetof(DotInstance, color) == 12);
static_assert(offsetof(DotInstance, diameterMeters) == 28);

// Single instance-rate binding, six procedural vertices per instance.
inline VkVertexInputBindingDescription dotsInstanceBinding() {
    return {0, sizeof(DotInstance), VK_VERTEX_INPUT_RATE_INSTANCE};
}
inline std::array<VkVertexInputAttributeDescription, 3> dotsInstanceAttributes() {
    return {{{0,0,VK_FORMAT_R32G32B32_SFLOAT,0},
             {1,0,VK_FORMAT_R32G32B32A32_SFLOAT,12},
             {2,0,VK_FORMAT_R32_SFLOAT,28}}};
}
class DotsGpuBuffer {
public:
    DotsGpuBuffer() = default;
    DotsGpuBuffer(const DotsGpuBuffer&) = delete;
    DotsGpuBuffer& operator=(const DotsGpuBuffer&) = delete;
    ~DotsGpuBuffer() = default; // Explicit destroy(device), like other renderer resources.

    VkBuffer buffer() const { return buffer_; }
    std::uint32_t count() const { return count_; }

    void upload(VkDevice device, VkPhysicalDevice physicalDevice,
                const std::vector<DotInstance>& instances) {
        if (buffer_ != VK_NULL_HANDLE)
            throw std::logic_error("DotsGpuBuffer: destroy old buffer before upload");
        if (instances.empty()) return;
        if (instances.size() > std::numeric_limits<std::uint32_t>::max() ||
            instances.size() > std::numeric_limits<VkDeviceSize>::max() / sizeof(DotInstance))
            throw std::length_error("DotsGpuBuffer: too many instances");
        const VkDeviceSize bytes = instances.size() * sizeof(DotInstance);
        VkBufferCreateInfo info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        info.size = bytes;
        info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        VkBuffer buffer = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        auto check = [](VkResult result, const char* message) {
            if (result != VK_SUCCESS) throw std::runtime_error(message);
        };
        try {
            check(vkCreateBuffer(device, &info, nullptr, &buffer),
                  "DotsGpuBuffer: vkCreateBuffer");
            VkMemoryRequirements req{};
            vkGetBufferMemoryRequirements(device, buffer, &req);
            VkPhysicalDeviceMemoryProperties properties{};
            vkGetPhysicalDeviceMemoryProperties(physicalDevice, &properties);
            std::uint32_t type = UINT32_MAX;
            for (std::uint32_t i = 0; i < properties.memoryTypeCount; ++i) {
                constexpr VkMemoryPropertyFlags wanted =
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
                if ((req.memoryTypeBits & (1u << i)) &&
                    (properties.memoryTypes[i].propertyFlags & wanted) == wanted) {
                    type = i;
                    break;
                }
            }
            if (type == UINT32_MAX)
                throw std::runtime_error("DotsGpuBuffer: no coherent host-visible memory");
            VkMemoryAllocateInfo alloc{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
            alloc.allocationSize = req.size;
            alloc.memoryTypeIndex = type;
            check(vkAllocateMemory(device, &alloc, nullptr, &memory),
                  "DotsGpuBuffer: vkAllocateMemory");
            check(vkBindBufferMemory(device, buffer, memory, 0),
                  "DotsGpuBuffer: vkBindBufferMemory");
            void* mapped = nullptr;
            check(vkMapMemory(device, memory, 0, bytes, 0, &mapped),
                  "DotsGpuBuffer: vkMapMemory");
            std::memcpy(mapped, instances.data(), static_cast<std::size_t>(bytes));
            vkUnmapMemory(device, memory);
            buffer_ = buffer;
            memory_ = memory;
            count_ = static_cast<std::uint32_t>(instances.size());
        } catch (...) {
            if (buffer) vkDestroyBuffer(device, buffer, nullptr);
            if (memory) vkFreeMemory(device, memory, nullptr);
            throw;
        }
    }

    void destroy(VkDevice device) {
        if (buffer_) vkDestroyBuffer(device, buffer_, nullptr);
        if (memory_) vkFreeMemory(device, memory_, nullptr);
        buffer_ = VK_NULL_HANDLE;
        memory_ = VK_NULL_HANDLE;
        count_ = 0;
    }

private:
    VkBuffer buffer_{VK_NULL_HANDLE};
    VkDeviceMemory memory_{VK_NULL_HANDLE};
    std::uint32_t count_{0};
};
} // namespace voxen::render::openbrush
