#pragma once
#include <voxen/render/OpenBrushDotsGpu.hpp>
#include <array>
#include <cstddef>
#include <vulkan/vulkan.h>

namespace voxen::render::openbrush {
// Column-major matrix, consistent with GLSL mat4. Right/up must be normalized
// world-space camera basis vectors for the current eye.
struct alignas(16) DotsCameraPush {
    std::array<float,16> viewProjection{};
    std::array<float,4> cameraRight{};
    std::array<float,4> cameraUp{};
};
static_assert(sizeof(DotsCameraPush) == 96);
static_assert(offsetof(DotsCameraPush, cameraRight) == 64);
static_assert(offsetof(DotsCameraPush, cameraUp) == 80);

inline VkPushConstantRange dotsCameraPushRange() {
    return {VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(DotsCameraPush)};
}

// Called INSIDE an active compatible render pass, once for each eye.
// Pipeline must use dotsInstanceBinding()/dotsInstanceAttributes(), triangle
// list, depth test enabled / depth writes disabled, and premultiplied alpha:
// srcColor=ONE, dstColor=ONE_MINUS_SRC_ALPHA.
inline void recordDots(VkCommandBuffer cmd, VkPipeline pipeline,
                       VkPipelineLayout layout, const DotsGpuBuffer& dots,
                       const DotsCameraPush& camera) {
    if (dots.count() == 0 || dots.buffer() == VK_NULL_HANDLE ||
        pipeline == VK_NULL_HANDLE || layout == VK_NULL_HANDLE) return;
    VkDeviceSize offset = 0;
    VkBuffer buffer = dots.buffer();
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    vkCmdBindVertexBuffers(cmd, 0, 1, &buffer, &offset);
    vkCmdPushConstants(cmd, layout, VK_SHADER_STAGE_VERTEX_BIT,
                       0, sizeof(camera), &camera);
    vkCmdDraw(cmd, 6, dots.count(), 0, 0);
}
} // namespace voxen::render::openbrush
