#include <voxen/render/XrSwapchainRenderer.hpp>

#include <openxr/openxr_platform.h>

#ifdef __ANDROID__
#include <android/log.h>
#else
#include <cstdio>
#endif

#include "cubeVert.spv.hpp"
#include "cubeFrag.spv.hpp"

#include <algorithm>
#include <array>
#include <stdexcept>
#include <string>
#include <vector>

namespace voxen::render {
namespace {

void checkXr(XrResult result, const char* operation) {
    if (XR_FAILED(result)) {
        throw std::runtime_error(std::string(operation) + " failed with XrResult " + std::to_string(result));
    }
}

void checkVk(VkResult result, const char* operation) {
    if (result != VK_SUCCESS) {
        throw std::runtime_error(std::string(operation) + " failed with VkResult " + std::to_string(result));
    }
}

} // namespace

XrSwapchainRenderer::~XrSwapchainRenderer() {
    shutdown();
}

void XrSwapchainRenderer::initialize(
    XrInstance instance,
    XrSystemId systemId,
    ::XrSession session,
    VkDevice device,
    VkQueue queue,
    uint32_t queueFamilyIndex) {
    session_ = session;
    device_ = device;
    queue_ = queue;
    queueFamilyIndex_ = queueFamilyIndex;

    createCommandResources();
    createSwapchains(instance, systemId, session);
    createPipeline();
    createFramebuffers();
}

void XrSwapchainRenderer::createCommandResources() {
    VkCommandPoolCreateInfo poolInfo{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamilyIndex_;
    checkVk(vkCreateCommandPool(device_, &poolInfo, nullptr, &commandPool_), "vkCreateCommandPool");

    VkCommandBufferAllocateInfo allocInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    allocInfo.commandPool = commandPool_;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;
    checkVk(vkAllocateCommandBuffers(device_, &allocInfo, &commandBuffer_), "vkAllocateCommandBuffers");

    VkFenceCreateInfo fenceInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    checkVk(vkCreateFence(device_, &fenceInfo, nullptr, &fence_), "vkCreateFence");
}

void XrSwapchainRenderer::createSwapchains(
    XrInstance instance,
    XrSystemId systemId,
    ::XrSession session) {
    uint32_t formatCount = 0;
    checkXr(xrEnumerateSwapchainFormats(session, 0, &formatCount, nullptr), "xrEnumerateSwapchainFormats(count)");
    std::vector<int64_t> formats(formatCount);
    checkXr(xrEnumerateSwapchainFormats(session, formatCount, &formatCount, formats.data()), "xrEnumerateSwapchainFormats");

    const std::array<VkFormat, 3> preferred = {
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_FORMAT_B8G8R8A8_SRGB,
        VK_FORMAT_R8G8B8A8_UNORM,
    };
    for (const auto candidate : preferred) {
        if (std::find(formats.begin(), formats.end(), static_cast<int64_t>(candidate)) != formats.end()) {
            colorFormat_ = candidate;
            break;
        }
    }
    if (colorFormat_ == VK_FORMAT_UNDEFINED) {
        throw std::runtime_error("No supported RGBA OpenXR swapchain format");
    }

    uint32_t viewCount = 0;
    checkXr(
        xrEnumerateViewConfigurationViews(
            instance, systemId, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, 0, &viewCount, nullptr),
        "xrEnumerateViewConfigurationViews(count)");
    if (viewCount != 2) {
        throw std::runtime_error("Voxen currently requires exactly two stereo views");
    }

    std::array<XrViewConfigurationView, 2> configs = {
        XrViewConfigurationView{XR_TYPE_VIEW_CONFIGURATION_VIEW},
        XrViewConfigurationView{XR_TYPE_VIEW_CONFIGURATION_VIEW},
    };
    checkXr(
        xrEnumerateViewConfigurationViews(
            instance,
            systemId,
            XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
            static_cast<uint32_t>(configs.size()),
            &viewCount,
            configs.data()),
        "xrEnumerateViewConfigurationViews");

    for (uint32_t eye = 0; eye < eyes_.size(); ++eye) {
        auto& target = eyes_[eye];
        target.width = static_cast<int32_t>(configs[eye].recommendedImageRectWidth);
        target.height = static_cast<int32_t>(configs[eye].recommendedImageRectHeight);

        XrSwapchainCreateInfo info{XR_TYPE_SWAPCHAIN_CREATE_INFO};
        info.usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT | XR_SWAPCHAIN_USAGE_TRANSFER_DST_BIT;
        info.format = static_cast<int64_t>(colorFormat_);
        info.sampleCount = 1;
        info.width = configs[eye].recommendedImageRectWidth;
        info.height = configs[eye].recommendedImageRectHeight;
        info.faceCount = 1;
        info.arraySize = 1;
        info.mipCount = 1;
        checkXr(xrCreateSwapchain(session, &info, &target.handle), "xrCreateSwapchain");

        uint32_t imageCount = 0;
        checkXr(xrEnumerateSwapchainImages(target.handle, 0, &imageCount, nullptr), "xrEnumerateSwapchainImages(count)");
        target.images.assign(imageCount, {XR_TYPE_SWAPCHAIN_IMAGE_VULKAN2_KHR});
        target.layouts.assign(imageCount, VK_IMAGE_LAYOUT_UNDEFINED);
        checkXr(
            xrEnumerateSwapchainImages(
                target.handle,
                imageCount,
                &imageCount,
                reinterpret_cast<XrSwapchainImageBaseHeader*>(target.images.data())),
            "xrEnumerateSwapchainImages");
    }
}

namespace {

struct Mat4 { float m[16]; };

Mat4 multiply(const Mat4& a, const Mat4& b) {
    Mat4 r{};
    for (int col = 0; col < 4; ++col)
        for (int row = 0; row < 4; ++row)
            for (int k = 0; k < 4; ++k)
                r.m[col * 4 + row] += a.m[k * 4 + row] * b.m[col * 4 + k];
    return r;
}

Mat4 projection(const XrFovf& fov, float nearZ, float farZ) {
    const float l = std::tan(fov.angleLeft);
    const float r = std::tan(fov.angleRight);
    const float d = std::tan(fov.angleDown);
    const float u = std::tan(fov.angleUp);
    Mat4 m{};
    m.m[0] = 2.0F / (r - l);
    m.m[5] = 2.0F / (u - d);
    m.m[8] = (r + l) / (r - l);
    m.m[9] = (u + d) / (u - d);
    m.m[10] = -farZ / (farZ - nearZ);
    m.m[11] = -1.0F;
    m.m[14] = -(farZ * nearZ) / (farZ - nearZ);
    return m;
}

Mat4 viewMatrix(const XrPosef& pose) {
    // xrLocateViews returns the eye pose in reference-space coordinates.
    // The view matrix is the inverse rigid transform: R(conjugate(q)) * T(-p).
    const float x = -pose.orientation.x;
    const float y = -pose.orientation.y;
    const float z = -pose.orientation.z;
    const float w = pose.orientation.w;

    Mat4 rot{};
    rot.m[0]=1-2*y*y-2*z*z; rot.m[1]=2*x*y+2*z*w; rot.m[2]=2*x*z-2*y*w;
    rot.m[4]=2*x*y-2*z*w; rot.m[5]=1-2*x*x-2*z*z; rot.m[6]=2*y*z+2*x*w;
    rot.m[8]=2*x*z+2*y*w; rot.m[9]=2*y*z-2*x*w; rot.m[10]=1-2*x*x-2*y*y;
    rot.m[15]=1.0F;

    Mat4 t{};
    t.m[0]=t.m[5]=t.m[10]=t.m[15]=1.0F;
    t.m[12]=-pose.position.x;
    t.m[13]=-pose.position.y;
    t.m[14]=-pose.position.z;
    return multiply(rot, t);
}

Mat4 modelMatrix() {
    Mat4 m{};
    m.m[0]=m.m[5]=m.m[10]=0.25F;
    m.m[15]=1.0F;
    m.m[14]=-1.5F;
    return m;
}

VkShaderModule makeShader(VkDevice device, const std::uint8_t* bytes, std::size_t size) {
    VkShaderModuleCreateInfo info{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    info.codeSize = size;
    info.pCode = reinterpret_cast<const uint32_t*>(bytes);
    VkShaderModule module = VK_NULL_HANDLE;
    checkVk(vkCreateShaderModule(device, &info, nullptr, &module), "vkCreateShaderModule");
    return module;
}

} // namespace

void XrSwapchainRenderer::createPipeline() {
    VkAttachmentDescription color{};
    color.format = colorFormat_;
    color.samples = VK_SAMPLE_COUNT_1_BIT;
    color.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    color.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference ref{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &ref;

    VkRenderPassCreateInfo rp{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    rp.attachmentCount = 1; rp.pAttachments = &color;
    rp.subpassCount = 1; rp.pSubpasses = &subpass;
    checkVk(vkCreateRenderPass(device_, &rp, nullptr, &renderPass_), "vkCreateRenderPass");

    VkPushConstantRange push{};
    push.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    push.size = sizeof(Mat4);
    VkPipelineLayoutCreateInfo pli{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    pli.pushConstantRangeCount = 1; pli.pPushConstantRanges = &push;
    checkVk(vkCreatePipelineLayout(device_, &pli, nullptr, &pipelineLayout_), "vkCreatePipelineLayout");

    VkShaderModule vs = makeShader(device_, cubeVert, cubeVertSize);
    VkShaderModule fs = makeShader(device_, cubeFrag, cubeFragSize);
    VkPipelineShaderStageCreateInfo stages[2]{};
    stages[0] = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    stages[0].stage=VK_SHADER_STAGE_VERTEX_BIT; stages[0].module=vs; stages[0].pName="main";
    stages[1] = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    stages[1].stage=VK_SHADER_STAGE_FRAGMENT_BIT; stages[1].module=fs; stages[1].pName="main";

    VkPipelineVertexInputStateCreateInfo vi{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    VkPipelineInputAssemblyStateCreateInfo ia{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    VkPipelineViewportStateCreateInfo vp{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    vp.viewportCount=1; vp.scissorCount=1;
    VkPipelineRasterizationStateCreateInfo rs{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    rs.polygonMode=VK_POLYGON_MODE_FILL; rs.cullMode=VK_CULL_MODE_NONE;
    rs.frontFace=VK_FRONT_FACE_COUNTER_CLOCKWISE; rs.lineWidth=1.0F;
    VkPipelineMultisampleStateCreateInfo ms{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    ms.rasterizationSamples=VK_SAMPLE_COUNT_1_BIT;
    VkPipelineColorBlendAttachmentState blend{}; blend.colorWriteMask=0xF;
    VkPipelineColorBlendStateCreateInfo cb{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    cb.attachmentCount=1; cb.pAttachments=&blend;
    VkDynamicState dyns[]={VK_DYNAMIC_STATE_VIEWPORT,VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dyn{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
    dyn.dynamicStateCount=2; dyn.pDynamicStates=dyns;

    VkGraphicsPipelineCreateInfo pi{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    pi.stageCount=2; pi.pStages=stages; pi.pVertexInputState=&vi; pi.pInputAssemblyState=&ia;
    pi.pViewportState=&vp; pi.pRasterizationState=&rs; pi.pMultisampleState=&ms;
    pi.pColorBlendState=&cb; pi.pDynamicState=&dyn; pi.layout=pipelineLayout_; pi.renderPass=renderPass_;
    checkVk(vkCreateGraphicsPipelines(device_, VK_NULL_HANDLE, 1, &pi, nullptr, &pipeline_), "vkCreateGraphicsPipelines");
    vkDestroyShaderModule(device_, fs, nullptr);
    vkDestroyShaderModule(device_, vs, nullptr);
}

void XrSwapchainRenderer::createFramebuffers() {
    for (auto& eye : eyes_) {
        eye.views.resize(eye.images.size());
        eye.framebuffers.resize(eye.images.size());
        for (std::size_t i=0; i<eye.images.size(); ++i) {
            VkImageViewCreateInfo iv{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
            iv.image=eye.images[i].image; iv.viewType=VK_IMAGE_VIEW_TYPE_2D; iv.format=colorFormat_;
            iv.subresourceRange.aspectMask=VK_IMAGE_ASPECT_COLOR_BIT; iv.subresourceRange.levelCount=1; iv.subresourceRange.layerCount=1;
            checkVk(vkCreateImageView(device_, &iv, nullptr, &eye.views[i]), "vkCreateImageView");
            VkFramebufferCreateInfo fb{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
            fb.renderPass=renderPass_; fb.attachmentCount=1; fb.pAttachments=&eye.views[i];
            fb.width=eye.width; fb.height=eye.height; fb.layers=1;
            checkVk(vkCreateFramebuffer(device_, &fb, nullptr, &eye.framebuffers[i]), "vkCreateFramebuffer");
        }
    }
}

void XrSwapchainRenderer::renderImage(EyeSwapchain& eye, uint32_t imageIndex, const XrView& xrView) {
    checkVk(vkResetFences(device_, 1, &fence_), "vkResetFences");
    checkVk(vkResetCommandBuffer(commandBuffer_, 0), "vkResetCommandBuffer");
    VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin.flags=VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    checkVk(vkBeginCommandBuffer(commandBuffer_, &begin), "vkBeginCommandBuffer");

    auto& layout=eye.layouts.at(imageIndex);
    if (layout != VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
        VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        barrier.oldLayout=layout; barrier.newLayout=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        barrier.srcQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED; barrier.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
        barrier.image=eye.images.at(imageIndex).image;
        barrier.subresourceRange.aspectMask=VK_IMAGE_ASPECT_COLOR_BIT; barrier.subresourceRange.levelCount=1; barrier.subresourceRange.layerCount=1;
        barrier.dstAccessMask=VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        vkCmdPipelineBarrier(commandBuffer_, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                             0,0,nullptr,0,nullptr,1,&barrier);
        layout=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }

    VkClearValue clear{}; clear.color.float32[0]=0.08F; clear.color.float32[1]=0.12F; clear.color.float32[2]=0.22F; clear.color.float32[3]=0.0F;
    VkRenderPassBeginInfo rbi{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    rbi.renderPass=renderPass_; rbi.framebuffer=eye.framebuffers.at(imageIndex);
    rbi.renderArea.extent={static_cast<uint32_t>(eye.width),static_cast<uint32_t>(eye.height)};
    rbi.clearValueCount=1; rbi.pClearValues=&clear;
    vkCmdBeginRenderPass(commandBuffer_, &rbi, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport{0,0,static_cast<float>(eye.width),static_cast<float>(eye.height),0,1};
    VkRect2D scissor{{0,0},{static_cast<uint32_t>(eye.width),static_cast<uint32_t>(eye.height)}};
    vkCmdSetViewport(commandBuffer_,0,1,&viewport); vkCmdSetScissor(commandBuffer_,0,1,&scissor);
    vkCmdBindPipeline(commandBuffer_,VK_PIPELINE_BIND_POINT_GRAPHICS,pipeline_);

    const Mat4 mvp=multiply(projection(xrView.fov,0.05F,100.0F),multiply(viewMatrix(xrView.pose),modelMatrix()));
    vkCmdPushConstants(commandBuffer_,pipelineLayout_,VK_SHADER_STAGE_VERTEX_BIT,0,sizeof(mvp),&mvp);
    vkCmdDraw(commandBuffer_,36,1,0,0);
    vkCmdEndRenderPass(commandBuffer_);
    checkVk(vkEndCommandBuffer(commandBuffer_), "vkEndCommandBuffer");
    VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO}; submit.commandBufferCount=1; submit.pCommandBuffers=&commandBuffer_;
    checkVk(vkQueueSubmit(queue_,1,&submit,fence_), "vkQueueSubmit");
    checkVk(vkWaitForFences(device_,1,&fence_,VK_TRUE,UINT64_MAX), "vkWaitForFences");
}

void XrSwapchainRenderer::render(
    const std::array<XrView, 2>& views,
    std::array<XrCompositionLayerProjectionView, 2>& projectionViews) {
    if (!firstFrameLogged_) {
#ifdef __ANDROID__
        __android_log_print(ANDROID_LOG_INFO, "Voxen", "Rendering first stereo projection frame");
#else
        std::printf("[Voxen] Rendering first stereo projection frame\n");
#endif
    }
    for (uint32_t eye = 0; eye < eyes_.size(); ++eye) {
        auto& swapchain = eyes_[eye];

        XrSwapchainImageAcquireInfo acquireInfo{XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
        uint32_t imageIndex = 0;
        checkXr(xrAcquireSwapchainImage(swapchain.handle, &acquireInfo, &imageIndex), "xrAcquireSwapchainImage");

        XrSwapchainImageWaitInfo waitInfo{XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};
        waitInfo.timeout = XR_INFINITE_DURATION;
        checkXr(xrWaitSwapchainImage(swapchain.handle, &waitInfo), "xrWaitSwapchainImage");

        renderImage(swapchain, imageIndex, views[eye]);

        XrSwapchainImageReleaseInfo releaseInfo{XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
        checkXr(xrReleaseSwapchainImage(swapchain.handle, &releaseInfo), "xrReleaseSwapchainImage");

        auto& projection = projectionViews[eye];
        projection = {XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW};
        projection.pose = views[eye].pose;
        projection.fov = views[eye].fov;
        projection.subImage.swapchain = swapchain.handle;
        projection.subImage.imageRect.offset = {0, 0};
        projection.subImage.imageRect.extent = {swapchain.width, swapchain.height};
        projection.subImage.imageArrayIndex = 0;
    }

    if (!firstFrameLogged_) {
#ifdef __ANDROID__
        __android_log_print(ANDROID_LOG_INFO, "Voxen", "First stereo projection frame rendered");
#else
        std::printf("[Voxen] First stereo projection frame rendered\n");
#endif
        firstFrameLogged_ = true;
    }
}

void XrSwapchainRenderer::shutdown() noexcept {
    if (device_ != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(device_);
    }

    for (auto& eye : eyes_) {
        for (auto fb : eye.framebuffers) if (fb != VK_NULL_HANDLE) vkDestroyFramebuffer(device_, fb, nullptr);
        for (auto view : eye.views) if (view != VK_NULL_HANDLE) vkDestroyImageView(device_, view, nullptr);
        eye.framebuffers.clear(); eye.views.clear();
        if (eye.handle != XR_NULL_HANDLE) {
            xrDestroySwapchain(eye.handle);
            eye.handle = XR_NULL_HANDLE;
        }
        eye.images.clear();
        eye.layouts.clear();
    }

    if (pipeline_ != VK_NULL_HANDLE) { vkDestroyPipeline(device_, pipeline_, nullptr); pipeline_ = VK_NULL_HANDLE; }
    if (pipelineLayout_ != VK_NULL_HANDLE) { vkDestroyPipelineLayout(device_, pipelineLayout_, nullptr); pipelineLayout_ = VK_NULL_HANDLE; }
    if (renderPass_ != VK_NULL_HANDLE) { vkDestroyRenderPass(device_, renderPass_, nullptr); renderPass_ = VK_NULL_HANDLE; }

    if (fence_ != VK_NULL_HANDLE) {
        vkDestroyFence(device_, fence_, nullptr);
        fence_ = VK_NULL_HANDLE;
    }
    if (commandPool_ != VK_NULL_HANDLE) {
        vkDestroyCommandPool(device_, commandPool_, nullptr);
        commandPool_ = VK_NULL_HANDLE;
    }

    commandBuffer_ = VK_NULL_HANDLE;
    queue_ = VK_NULL_HANDLE;
    device_ = VK_NULL_HANDLE;
    session_ = XR_NULL_HANDLE;
}

} // namespace voxen::render
