#include <voxen/render/XrSwapchainRenderer.hpp>
#ifdef VOXEN_WITH_IMGUI
#include <imgui.h>
#include <backends/imgui_impl_vulkan.h>
#include <voxen/ui/CarouselUi.hpp>
#include <voxen/ui/SpatialPanelInput.hpp>
#endif

#ifdef _WIN32
#include <windows.h>
#include <unknwn.h>
#endif
#include <openxr/openxr_platform.h>

#ifdef __ANDROID__
#include <android/log.h>
#else
#include <cstdio>
#endif

#include "cubeVert.spv.hpp"
#include "voxenPlasmaFrag.spv.hpp"
#include "voxenSpheresFrag.spv.hpp"
#include "voxenMandelbulbFrag.spv.hpp"
#include "voxenAttractorVert.spv.hpp"
#include "voxenAttractorFrag.spv.hpp"
#include "fractalVert.spv.hpp"
#include "laserFrag.spv.hpp"
#include "liquidRoomFrag.spv.hpp"
#include "solidWallFrag.spv.hpp"
#include "meshWireVert.spv.hpp"
#include "meshWireFrag.spv.hpp"
#include "meshDepthVert.spv.hpp"
#include "meshDepthFrag.spv.hpp"
#include "matrixRainFrag.spv.hpp"
#include "heppGlowFrag.spv.hpp"
#include "phiFieldFrag.spv.hpp"
#include "mandelbrotZoomFrag.spv.hpp"
#include "carouselFrag.spv.hpp"
#include "imguiPanelFrag.spv.hpp"

#include <algorithm>
#include <cmath>
#include <array>
#include <cstring>
#include <cfloat>
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
    VkInstance vulkanInstance,
    ::XrSession session,
    VkDevice device,
    VkPhysicalDevice physicalDevice,
    VkQueue queue,
    uint32_t queueFamilyIndex) {
    session_ = session;
    device_ = device;
    physicalDevice_ = physicalDevice;
    queue_ = queue;

    for (VkFormat candidate : {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT}) {
        VkFormatProperties properties{};
        vkGetPhysicalDeviceFormatProperties(physicalDevice_, candidate, &properties);
        if (properties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
            depthFormat_ = candidate;
            break;
        }
    }
    if (depthFormat_ == VK_FORMAT_UNDEFINED) throw std::runtime_error("No Vulkan depth attachment format supported");
    queueFamilyIndex_ = queueFamilyIndex;

    VkPhysicalDeviceProperties gpuProps{};
    vkGetPhysicalDeviceProperties(physicalDevice_, &gpuProps);
    uint32_t familyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice_, &familyCount, nullptr);
    std::vector<VkQueueFamilyProperties> families(familyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice_, &familyCount, families.data());
    if (queueFamilyIndex_ < familyCount &&
        families[queueFamilyIndex_].timestampValidBits != 0 &&
        gpuProps.limits.timestampComputeAndGraphics == VK_TRUE) {
        VkQueryPoolCreateInfo qi{VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO};
        qi.queryType = VK_QUERY_TYPE_TIMESTAMP;
        qi.queryCount = 2;
        if (vkCreateQueryPool(device_, &qi, nullptr, &gpuTimestampPool_) == VK_SUCCESS)
            timestampPeriodNs_ = gpuProps.limits.timestampPeriod;
    }
    ambientMusic_.start();
    createCommandResources();
    createSwapchains(instance, systemId, session);
#ifdef VOXEN_WITH_IMGUI
    // CPU-side ImGui lifecycle only. GPU texture compositing is a separate step.
    // Do not claim the temporary GLSL panel has been replaced yet.
    if (ImGui::GetCurrentContext() == nullptr) {
        ImGui::CreateContext();
        imguiContextOwned_ = true;
    }
    ImGui::StyleColorsDark();
    auto& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(512.0f, 672.0f);
    io.DeltaTime = 1.f / 72.f;
    io.IniFilename = nullptr;
    // Fetch the rasterized font atlas before the backend uploads it.
    io.Fonts->AddFontDefault();
    io.Fonts->Build();
    imguiSurface_.initialize(vulkanInstance,physicalDevice_,device_,queue_,queueFamilyIndex_);
#endif
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

struct ProceduralPushConstants {
    Mat4 mvp;
    float parameters[4]; // elapsed time, speed, spatial scale, reserved
};
static_assert(sizeof(ProceduralPushConstants) == 80);

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

Mat4 modelMatrix(float x = 0.0F, float y = 0.0F, float scale = 0.25F, float z = -1.5F, float yaw = 0.0F) {
    Mat4 m{};
    const float c = std::cos(yaw), si = std::sin(yaw);
    m.m[0]=c*scale; m.m[2]=-si*scale;
    m.m[5]=scale;
    m.m[8]=si*scale; m.m[10]=c*scale;
    m.m[12]=x;
    m.m[13]=y;
    m.m[15]=1.0F;
    m.m[14]=z;
    return m;
}

// Room surfaces arrive from the Scene provider in LOCAL space (meters).
// Transform the quad's local XY coordinates into the oriented tracked plane.
Mat4 surfaceModel(const voxen::scene::EnvironmentSurface& surface) {
    const auto& q=surface.orientation;
    const float x=q[0],y=q[1],z=q[2],w=q[3];
    Mat4 m{};
    m.m[0]=(1.f-2.f*(y*y+z*z))*surface.halfExtents[0];
    m.m[1]=(2.f*(x*y+z*w))*surface.halfExtents[0];
    m.m[2]=(2.f*(x*z-y*w))*surface.halfExtents[0];
    m.m[4]=(2.f*(x*y-z*w))*surface.halfExtents[1];
    m.m[5]=(1.f-2.f*(x*x+z*z))*surface.halfExtents[1];
    m.m[6]=(2.f*(y*z+x*w))*surface.halfExtents[1];
    m.m[8]=2.f*(x*z+y*w);
    m.m[9]=2.f*(y*z-x*w);
    m.m[10]=1.f-2.f*(x*x+y*y);
    m.m[12]=surface.center[0];
    m.m[13]=surface.center[1];
    m.m[14]=surface.center[2];
    m.m[15]=1.f;
    return m;
}

// A slender cube aligned with the controller's local -Z aiming direction.
Mat4 laserMatrix(const XrPosef& pose, float distance) {
    const auto& q = pose.orientation;
    const float dx = -2.0F * (q.x*q.z + q.w*q.y);
    const float dy = -2.0F * (q.y*q.z - q.w*q.x);
    const float dz = -(1.0F - 2.0F * (q.x*q.x + q.y*q.y));
    // Take a perpendicular sideways basis, then a second perpendicular axis.
    float sx = dz, sy = 0.0F, sz = -dx;
    const float sl = std::sqrt(sx*sx + sz*sz);
    if (sl < 0.001F) { sx=1.0F; sy=0.0F; sz=0.0F; }
    else { sx /= sl; sz /= sl; }
    const float ux = dy*sz - dz*sy;
    const float uy = dz*sx - dx*sz;
    const float uz = dx*sy - dy*sx;
    constexpr float width = 0.006F;
    Mat4 m{};
    m.m[0]=sx*width; m.m[1]=sy*width; m.m[2]=sz*width;
    m.m[4]=ux*width; m.m[5]=uy*width; m.m[6]=uz*width;
    m.m[8]=dx*distance; m.m[9]=dy*distance; m.m[10]=dz*distance;
    m.m[12]=pose.position.x+dx*distance*0.5F;
    m.m[13]=pose.position.y+dy*distance*0.5F;
    m.m[14]=pose.position.z+dz*distance*0.5F;
    m.m[15]=1.0F;
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

// Mesh edges are uploaded once and the changing anchor pose is applied by MVP.
static Mat4 meshPoseMatrix(const XrPosef& pose) {
    voxen::scene::EnvironmentSurface proxy{};
    proxy.center={pose.position.x,pose.position.y,pose.position.z};
    proxy.orientation={pose.orientation.x,pose.orientation.y,pose.orientation.z,pose.orientation.w};
    proxy.halfExtents={1.f,1.f};
    return surfaceModel(proxy);
}

void XrSwapchainRenderer::setSceneMeshes(const std::vector<MeshGeometry>& meshes) {
    if(meshes.empty())return;
    const auto& mesh=meshes.front();
    meshWirePose_=mesh.pose;
    if(meshDepthBuffer_!=VK_NULL_HANDLE || !mesh.vertices || !mesh.indices ||
       mesh.vertices->empty() || mesh.indices->empty())return;
    const auto& indices=*mesh.indices;
    const auto& vertices=*mesh.vertices;
    std::vector<XrVector3f> triangles;
    std::vector<XrVector3f> lines;
    triangles.reserve(indices.size());
    lines.reserve(indices.size()*2);
    for(size_t i=0;i+2<indices.size();i+=3){
        uint32_t a=indices[i],b=indices[i+1],c=indices[i+2];
        if(a>=vertices.size()||b>=vertices.size()||c>=vertices.size())continue;
        triangles.push_back(vertices[a]);triangles.push_back(vertices[b]);triangles.push_back(vertices[c]);
        lines.push_back(vertices[a]);lines.push_back(vertices[b]);
        lines.push_back(vertices[b]);lines.push_back(vertices[c]);
        lines.push_back(vertices[c]);lines.push_back(vertices[a]);
    }
    if(triangles.empty())return;
    VkPhysicalDeviceMemoryProperties properties{};
    vkGetPhysicalDeviceMemoryProperties(physicalDevice_,&properties);
    auto upload=[&](const std::vector<XrVector3f>& src,VkBuffer& buffer,VkDeviceMemory& memory) {
        const VkDeviceSize bytes=src.size()*sizeof(XrVector3f);
        VkBufferCreateInfo bi{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        bi.size=bytes;bi.usage=VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;bi.sharingMode=VK_SHARING_MODE_EXCLUSIVE;
        checkVk(vkCreateBuffer(device_,&bi,nullptr,&buffer),"vkCreateBuffer(scene mesh)");
        VkMemoryRequirements requirements{};
        vkGetBufferMemoryRequirements(device_,buffer,&requirements);
        uint32_t memoryType=UINT32_MAX;
        for(uint32_t i=0;i<properties.memoryTypeCount;i++){
            auto flags=properties.memoryTypes[i].propertyFlags;
            if((requirements.memoryTypeBits&(1u<<i)) &&
                (flags&(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))==
                (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)){
                memoryType=i;break;
            }
        }
        if(memoryType==UINT32_MAX)throw std::runtime_error("No coherent scene mesh memory");
        VkMemoryAllocateInfo ai{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
        ai.allocationSize=requirements.size;ai.memoryTypeIndex=memoryType;
        checkVk(vkAllocateMemory(device_,&ai,nullptr,&memory),"vkAllocateMemory(scene mesh)");
        checkVk(vkBindBufferMemory(device_,buffer,memory,0),"vkBindBufferMemory(scene mesh)");
        void* mapped=nullptr;
        checkVk(vkMapMemory(device_,memory,0,bytes,0,&mapped),"vkMapMemory(scene mesh)");
        std::memcpy(mapped,src.data(),static_cast<size_t>(bytes));
        vkUnmapMemory(device_,memory);
    };
    upload(triangles,meshDepthBuffer_,meshDepthMemory_);
    meshDepthVertexCount_=static_cast<uint32_t>(triangles.size());
    upload(lines,meshWireBuffer_,meshWireMemory_);
    meshWireVertexCount_=static_cast<uint32_t>(lines.size());
#ifdef __ANDROID__
    __android_log_print(ANDROID_LOG_INFO,"VoxenScene",
        "Scene depth upload: triangles=%u wire=%u",meshDepthVertexCount_/3,meshWireVertexCount_);
#endif
}

void XrSwapchainRenderer::createPipeline() {
    VkAttachmentDescription color{};
    color.format = colorFormat_;
    color.samples = VK_SAMPLE_COUNT_1_BIT;
    color.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    color.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription depth{};
    depth.format = depthFormat_;
    depth.samples = VK_SAMPLE_COUNT_1_BIT;
    depth.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depth.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depth.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference ref{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    VkAttachmentReference depthRef{1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &ref;
    subpass.pDepthStencilAttachment = &depthRef;

    VkRenderPassCreateInfo rp{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    const VkAttachmentDescription attachments[] = {color, depth};
    rp.attachmentCount = 2; rp.pAttachments = attachments;
    rp.subpassCount = 1; rp.pSubpasses = &subpass;
    checkVk(vkCreateRenderPass(device_, &rp, nullptr, &renderPass_), "vkCreateRenderPass");

    VkPushConstantRange push{};
    push.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    push.size = sizeof(ProceduralPushConstants);
    VkPipelineLayoutCreateInfo pli{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    pli.pushConstantRangeCount = 1; pli.pPushConstantRanges = &push;
    checkVk(vkCreatePipelineLayout(device_, &pli, nullptr, &pipelineLayout_), "vkCreatePipelineLayout");

#ifdef VOXEN_WITH_IMGUI
    // The offscreen texture descriptor layout is created by ImGuiSurface.
    VkPushConstantRange panelPush{};
    panelPush.stageFlags=VK_SHADER_STAGE_VERTEX_BIT;
    panelPush.size=sizeof(ProceduralPushConstants);
    VkDescriptorSetLayout uiSetLayout=imguiSurface_.descriptorLayout();
    VkPipelineLayoutCreateInfo panelInfo{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    panelInfo.setLayoutCount=1; panelInfo.pSetLayouts=&uiSetLayout;
    panelInfo.pushConstantRangeCount=1;panelInfo.pPushConstantRanges=&panelPush;
    checkVk(vkCreatePipelineLayout(device_,&panelInfo,nullptr,&imguiPanelPipelineLayout_),"vkCreatePipelineLayout(imgui)");
#endif
    // All three pipelines share a render pass and one 80-byte push constant layout.
    auto buildPipeline = [&](const std::uint8_t* vb, std::size_t vsz,
                             const std::uint8_t* fb, std::size_t fsz,
                             VkPipeline& out) {
        VkShaderModule vs = makeShader(device_, vb, vsz);
        VkShaderModule fs = makeShader(device_, fb, fsz);
        VkPipelineShaderStageCreateInfo stages[2]{};
        stages[0] = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
        stages[0].stage=VK_SHADER_STAGE_VERTEX_BIT; stages[0].module=vs; stages[0].pName="main";
        stages[1] = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
        stages[1].stage=VK_SHADER_STAGE_FRAGMENT_BIT; stages[1].module=fs; stages[1].pName="main";
        VkPipelineVertexInputStateCreateInfo vi{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
        VkPipelineInputAssemblyStateCreateInfo ia{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
        ia.topology = (&out==&meshWirePipeline_ || &out==&voxenAttractorPipeline_) ? VK_PRIMITIVE_TOPOLOGY_LINE_LIST : VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        VkVertexInputBindingDescription binding{0,sizeof(XrVector3f),VK_VERTEX_INPUT_RATE_VERTEX};
        VkVertexInputAttributeDescription attribute{0,0,VK_FORMAT_R32G32B32_SFLOAT,0};
        if(&out==&meshWirePipeline_ || &out==&meshDepthPipeline_) {
            vi.vertexBindingDescriptionCount=1;
            vi.pVertexBindingDescriptions=&binding;
            vi.vertexAttributeDescriptionCount=1;
            vi.pVertexAttributeDescriptions=&attribute;
        }
        VkPipelineViewportStateCreateInfo vp{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
        vp.viewportCount=1; vp.scissorCount=1;
        VkPipelineRasterizationStateCreateInfo rs{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
        rs.polygonMode=VK_POLYGON_MODE_FILL; rs.cullMode=VK_CULL_MODE_NONE;
        rs.frontFace=VK_FRONT_FACE_COUNTER_CLOCKWISE; rs.lineWidth=1.0F;
        VkPipelineMultisampleStateCreateInfo ms{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
        ms.rasterizationSamples=VK_SAMPLE_COUNT_1_BIT;
        VkPipelineDepthStencilStateCreateInfo ds{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
        ds.depthTestEnable = VK_TRUE;
        ds.depthWriteEnable = (&out == &laserPipeline_ || &out == &liquidRoomPipeline_ || &out == &solidWallPipeline_ || &out == &voxenPlasmaPipeline_ || &out == &voxenSpheresPipeline_ || &out == &voxenMandelbulbPipeline_ || &out == &voxenAttractorPipeline_ || &out == &meshWirePipeline_ || &out == &carouselPipeline_
#ifdef VOXEN_WITH_IMGUI
        || &out == &imguiPanelPipeline_
#endif
        ) ? VK_FALSE : VK_TRUE;
        if (&out == &carouselPipeline_
#ifdef VOXEN_WITH_IMGUI
            || &out == &imguiPanelPipeline_
#endif
        ) ds.depthTestEnable=VK_FALSE;
        if(&out==&meshDepthPipeline_){
            ds.depthTestEnable=VK_TRUE;
            ds.depthWriteEnable=VK_TRUE;
        }
        ds.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
        VkPipelineColorBlendAttachmentState blend{}; blend.colorWriteMask=0xF;
        if (&out == &liquidRoomPipeline_ || &out == &solidWallPipeline_ || &out == &matrixRainPipeline_ || &out == &heppGlowPipeline_ || &out == &phiFieldPipeline_ || &out == &mandelbrotZoomPipeline_ || &out == &voxenPlasmaPipeline_ || &out == &voxenSpheresPipeline_ || &out == &voxenMandelbulbPipeline_ || &out == &voxenAttractorPipeline_ || &out == &meshWirePipeline_ || &out == &carouselPipeline_
#ifdef VOXEN_WITH_IMGUI
            || &out == &imguiPanelPipeline_
#endif
        ) {
            blend.blendEnable=VK_TRUE;
            blend.srcColorBlendFactor=VK_BLEND_FACTOR_SRC_ALPHA;
            blend.dstColorBlendFactor=VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            blend.colorBlendOp=VK_BLEND_OP_ADD;
            blend.srcAlphaBlendFactor=VK_BLEND_FACTOR_ONE;
            blend.dstAlphaBlendFactor=VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            blend.alphaBlendOp=VK_BLEND_OP_ADD;
        }
        if(&out==&meshDepthPipeline_) {
            blend.colorWriteMask=0;
            blend.blendEnable=VK_FALSE;
        }
        VkPipelineColorBlendStateCreateInfo cb{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
        cb.attachmentCount=1; cb.pAttachments=&blend;
        VkDynamicState dyns[]={VK_DYNAMIC_STATE_VIEWPORT,VK_DYNAMIC_STATE_SCISSOR};
        VkPipelineDynamicStateCreateInfo dyn{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
        dyn.dynamicStateCount=2; dyn.pDynamicStates=dyns;
    
        VkGraphicsPipelineCreateInfo pi{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
        pi.stageCount=2; pi.pStages=stages; pi.pVertexInputState=&vi; pi.pInputAssemblyState=&ia;
        pi.pViewportState=&vp; pi.pRasterizationState=&rs; pi.pMultisampleState=&ms;
        pi.pColorBlendState=&cb; pi.pDepthStencilState=&ds; pi.pDynamicState=&dyn; pi.layout=pipelineLayout_;
#ifdef VOXEN_WITH_IMGUI
        if(&out==&imguiPanelPipeline_)pi.layout=imguiPanelPipelineLayout_;
#endif
        pi.renderPass=renderPass_;
        checkVk(vkCreateGraphicsPipelines(device_, VK_NULL_HANDLE, 1, &pi, nullptr, &out), "vkCreateGraphicsPipelines");
        vkDestroyShaderModule(device_, fs, nullptr);
        vkDestroyShaderModule(device_, vs, nullptr);
    };
    buildPipeline(fractalVert, fractalVertSize, voxenPlasmaFrag, voxenPlasmaFragSize, voxenPlasmaPipeline_);
    buildPipeline(fractalVert, fractalVertSize, voxenSpheresFrag, voxenSpheresFragSize, voxenSpheresPipeline_);
    buildPipeline(fractalVert, fractalVertSize, voxenMandelbulbFrag, voxenMandelbulbFragSize, voxenMandelbulbPipeline_);
    buildPipeline(voxenAttractorVert, voxenAttractorVertSize, voxenAttractorFrag, voxenAttractorFragSize, voxenAttractorPipeline_);
    buildPipeline(cubeVert, cubeVertSize, laserFrag, laserFragSize, laserPipeline_);
    buildPipeline(fractalVert, fractalVertSize, liquidRoomFrag, liquidRoomFragSize, liquidRoomPipeline_);
    buildPipeline(fractalVert, fractalVertSize, solidWallFrag, solidWallFragSize, solidWallPipeline_);
    buildPipeline(meshWireVert, meshWireVertSize, meshWireFrag, meshWireFragSize, meshWirePipeline_);
    buildPipeline(meshDepthVert, meshDepthVertSize, meshDepthFrag, meshDepthFragSize, meshDepthPipeline_);
    buildPipeline(fractalVert, fractalVertSize, matrixRainFrag, matrixRainFragSize, matrixRainPipeline_);
    buildPipeline(fractalVert, fractalVertSize, heppGlowFrag, heppGlowFragSize, heppGlowPipeline_);
    buildPipeline(fractalVert, fractalVertSize, phiFieldFrag, phiFieldFragSize, phiFieldPipeline_);
    buildPipeline(fractalVert, fractalVertSize, mandelbrotZoomFrag, mandelbrotZoomFragSize, mandelbrotZoomPipeline_);
    buildPipeline(fractalVert, fractalVertSize, carouselFrag, carouselFragSize, carouselPipeline_);
#ifdef VOXEN_WITH_IMGUI
    buildPipeline(fractalVert, fractalVertSize, imguiPanelFrag, imguiPanelFragSize, imguiPanelPipeline_);
#endif
}

void XrSwapchainRenderer::createFramebuffers() {
    VkPhysicalDeviceMemoryProperties memoryProperties{};
    vkGetPhysicalDeviceMemoryProperties(physicalDevice_, &memoryProperties);
    for (auto& eye : eyes_) {
        VkImageCreateInfo imageInfo{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.format = depthFormat_;
        imageInfo.extent = {static_cast<uint32_t>(eye.width), static_cast<uint32_t>(eye.height), 1};
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        checkVk(vkCreateImage(device_, &imageInfo, nullptr, &eye.depthImage), "vkCreateImage(depth)");

        VkMemoryRequirements requirements{};
        vkGetImageMemoryRequirements(device_, eye.depthImage, &requirements);
        uint32_t memoryType = UINT32_MAX;
        for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i) {
            if ((requirements.memoryTypeBits & (1u << i)) &&
                (memoryProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) {
                memoryType = i;
                break;
            }
        }
        if (memoryType == UINT32_MAX) throw std::runtime_error("No device-local memory type for depth image");
        VkMemoryAllocateInfo allocation{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
        allocation.allocationSize = requirements.size;
        allocation.memoryTypeIndex = memoryType;
        checkVk(vkAllocateMemory(device_, &allocation, nullptr, &eye.depthMemory), "vkAllocateMemory(depth)");
        checkVk(vkBindImageMemory(device_, eye.depthImage, eye.depthMemory, 0), "vkBindImageMemory(depth)");

        VkImageViewCreateInfo depthViewInfo{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        depthViewInfo.image = eye.depthImage;
        depthViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        depthViewInfo.format = depthFormat_;
        depthViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        depthViewInfo.subresourceRange.levelCount = 1;
        depthViewInfo.subresourceRange.layerCount = 1;
        checkVk(vkCreateImageView(device_, &depthViewInfo, nullptr, &eye.depthView), "vkCreateImageView(depth)");

        eye.views.resize(eye.images.size());
        eye.framebuffers.resize(eye.images.size());
        for (std::size_t i = 0; i < eye.images.size(); ++i) {
            VkImageViewCreateInfo iv{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
            iv.image = eye.images[i].image;
            iv.viewType = VK_IMAGE_VIEW_TYPE_2D;
            iv.format = colorFormat_;
            iv.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            iv.subresourceRange.levelCount = 1;
            iv.subresourceRange.layerCount = 1;
            checkVk(vkCreateImageView(device_, &iv, nullptr, &eye.views[i]), "vkCreateImageView(color)");

            const VkImageView attachments[] = {eye.views[i], eye.depthView};
            VkFramebufferCreateInfo fb{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
            fb.renderPass = renderPass_;
            fb.attachmentCount = 2;
            fb.pAttachments = attachments;
            fb.width = static_cast<uint32_t>(eye.width);
            fb.height = static_cast<uint32_t>(eye.height);
            fb.layers = 1;
            checkVk(vkCreateFramebuffer(device_, &fb, nullptr, &eye.framebuffers[i]), "vkCreateFramebuffer");
        }
    }
}

void XrSwapchainRenderer::updateControllerInteraction(
    const std::array<XrPosef, 2>& poses,
    const std::array<bool, 2>& tracked,
    const std::array<bool, 2>& pressed,
    const std::array<XrVector2f, 2>& thumbsticks) {
    controllerPoses_=poses;
    controllerTracked_=tracked;
#ifdef VOXEN_WITH_IMGUI
    uiTrigger_[0]=pressed[0];
    uiTrigger_[1]=pressed[1];
#endif
    if(tracked[0]) {
        float axis=thumbsticks[0].x;
        if(std::fabs(axis)<0.3f)nativeMenuStickLatched_=false;
        if(!nativeMenuStickLatched_ && std::fabs(axis)>0.70f){
            nativeMenuPage_=(nativeMenuPage_+(axis>0?1:2))%3;
            nativeMenuStickLatched_=true;
        }
    }
#ifdef VOXEN_WITH_IMGUI
    if(tracked[1] && pressed[1] && imguiCarousel_.voxenPaintEnabled && !panelAimed_){
        const auto& pose=poses[1];
        const auto& q=pose.orientation;
        const std::array<float,3> dir{
            -2.f*(q.x*q.z+q.w*q.y),
            -2.f*(q.y*q.z-q.w*q.x),
            -(1.f-2.f*(q.x*q.x+q.y*q.y))};
        PlasmaVoxen v;
        v.preset=imguiCarousel_.voxenPreset;
        v.position={pose.position.x+0.17f*dir[0],
                    pose.position.y+0.17f*dir[1],
                    pose.position.z+0.17f*dir[2]};
        v.scale=std::clamp(imguiCarousel_.voxenScale,0.06f,0.6f);
        v.hue=imguiCarousel_.voxenHue;
        v.density=imguiCarousel_.voxenDensity;
        v.speed=imguiCarousel_.voxenSpeed;
        const float dx=v.position[0]-lastVoxenPos_[0];
        const float dy=v.position[1]-lastVoxenPos_[1];
        const float dz=v.position[2]-lastVoxenPos_[2];
        if(!lastVoxenValid_ || dx*dx+dy*dy+dz*dz >= v.scale*v.scale*0.45f){
            if(voxens_.size()<256)voxens_.push_back(v);
            lastVoxenPos_=v.position;
            lastVoxenValid_=true;
        }
    }else if(!pressed[1])lastVoxenValid_=false;
#endif
}

void XrSwapchainRenderer::renderImage(EyeSwapchain& eye, uint32_t imageIndex, const XrView& xrView) {
    checkVk(vkResetFences(device_, 1, &fence_), "vkResetFences");
    checkVk(vkResetCommandBuffer(commandBuffer_, 0), "vkResetCommandBuffer");
    VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin.flags=VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    checkVk(vkBeginCommandBuffer(commandBuffer_, &begin), "vkBeginCommandBuffer");

    if (gpuTimestampPool_ != VK_NULL_HANDLE) {
        vkCmdResetQueryPool(commandBuffer_, gpuTimestampPool_, 0, 2);
        vkCmdWriteTimestamp(commandBuffer_, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, gpuTimestampPool_, 0);
    }

#ifdef VOXEN_WITH_IMGUI
    if (imguiSurface_.ready() && ImGui::GetDrawData()!=nullptr && &eye==&eyes_[0])
        imguiSurface_.render(commandBuffer_,ImGui::GetDrawData());
#endif
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
    VkClearValue clears[2]{};
    clears[0] = clear;
    clears[1].depthStencil = {1.0F, 0};
    rbi.clearValueCount=2; rbi.pClearValues=clears;
    vkCmdBeginRenderPass(commandBuffer_, &rbi, VK_SUBPASS_CONTENTS_INLINE);

    // OpenXR projection uses Y-up; Vulkan's positive-height viewport flips it.
    // Use a negative viewport height to retain the OpenXR world-space orientation.
    VkViewport viewport{0,static_cast<float>(eye.height),static_cast<float>(eye.width),-static_cast<float>(eye.height),0,1};
    VkRect2D scissor{{0,0},{static_cast<uint32_t>(eye.width),static_cast<uint32_t>(eye.height)}};
    vkCmdSetViewport(commandBuffer_,0,1,&viewport); vkCmdSetScissor(commandBuffer_,0,1,&scissor);

    const float seconds = std::chrono::duration<float>(
        std::chrono::steady_clock::now() - animationStart_).count();
    const Mat4 projView = multiply(projection(xrView.fov, 0.05F, 100.0F), viewMatrix(xrView.pose));
    // First visible Liquid Room demonstration: a synthetic, labelled quad,
    // NOT a detected wall. Spatial-surface binding comes in the next stage.
    bool showSceneMesh=false;
    bool sceneOcclusion=false;
    float sceneDepthBias=0.002f;
    bool liquidEnabled=true;
    float liquidOpacity=0.42f;
    int maxLiquidWalls=3;
    bool includeFloorCeiling=true;
    int wallShaderMode=0;
    bool cycleWallShaders=true;
    float wallCycleSeconds=30.f;
    std::array<float,3> wallColor{0.12f,0.55f,1.0f};
    float solidOpacity=0.85f;
    float globalSpeed=1.0f;
    bool artworksAnimated=true;
    int motionMode=0;
    float motionAmplitude=0.25f;
#ifdef VOXEN_WITH_IMGUI
    showSceneMesh=imguiCarousel_.showSceneMesh;
    sceneOcclusion=imguiCarousel_.sceneOcclusion;
    sceneDepthBias=std::clamp(imguiCarousel_.sceneDepthBias,0.f,0.01f);
    liquidEnabled=imguiCarousel_.liquidRoom;
    liquidOpacity=std::clamp(imguiCarousel_.liquidOpacity,0.0f,0.6f);
    maxLiquidWalls=std::clamp(imguiCarousel_.maxLiquidWalls,1,8);
    includeFloorCeiling=imguiCarousel_.includeFloorCeiling;
    wallShaderMode=imguiCarousel_.wallShaderMode;
    cycleWallShaders=imguiCarousel_.cycleWallShaders;
    wallCycleSeconds=std::clamp(imguiCarousel_.wallCycleSeconds,10.f,90.f);
    wallColor={imguiCarousel_.wallColor[0],imguiCarousel_.wallColor[1],imguiCarousel_.wallColor[2]};
    solidOpacity=imguiCarousel_.wallSolidOpacity;
    globalSpeed=imguiCarousel_.animationSpeed;
    artworksAnimated=imguiCarousel_.animateArtworks;
    motionMode=imguiCarousel_.artworkMotion;
    motionAmplitude=std::clamp(imguiCarousel_.motionAmplitude,0.0f,0.5f);
#endif
    // Depth-only static real-world mesh: occludes wall fragments behind
    // furniture, but never writes opaque pixels over the passthrough layer.
    if(sceneOcclusion && meshDepthVertexCount_>0 && meshDepthBuffer_!=VK_NULL_HANDLE) {
        vkCmdBindPipeline(commandBuffer_,VK_PIPELINE_BIND_POINT_GRAPHICS,meshDepthPipeline_);
        const VkDeviceSize offset=0;
        vkCmdBindVertexBuffers(commandBuffer_,0,1,&meshDepthBuffer_,&offset);
        const auto mvp=multiply(projView,meshPoseMatrix(meshWirePose_));
        ProceduralPushConstants depthConstants{mvp,{sceneDepthBias,0.f,0.f,0.f}};
        vkCmdPushConstants(commandBuffer_,pipelineLayout_,
            VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT,0,sizeof(depthConstants),&depthConstants);
        vkCmdDraw(commandBuffer_,meshDepthVertexCount_,1,0,0);
    }
    if (liquidEnabled && (wallShaderMode!=0 || liquidOpacity>0.001f) &&
        liquidRoomPipeline_ != VK_NULL_HANDLE && solidWallPipeline_ != VK_NULL_HANDLE && matrixRainPipeline_ != VK_NULL_HANDLE && heppGlowPipeline_ != VK_NULL_HANDLE && phiFieldPipeline_ != VK_NULL_HANDLE && mandelbrotZoomPipeline_ != VK_NULL_HANDLE) {
        // Stable slots are based on scene anchor IDs, not eye-specific
        // distance ordering. Both eyes therefore show the same effect per wall.
        std::vector<uint64_t> shaderIds;
        for(const auto& candidate:environmentSurfaces_.surfaces()) {
            if(candidate.tracked && candidate.classification!=voxen::scene::SurfaceClass::Unknown)
                shaderIds.push_back(candidate.id);
        }
        std::sort(shaderIds.begin(),shaderIds.end());
        shaderIds.erase(std::unique(shaderIds.begin(),shaderIds.end()),shaderIds.end());
        const int cycleStep=static_cast<int>(std::floor(seconds / wallCycleSeconds));
        constexpr int demoModes[]={0,2,3,4,5}; // never SOLID or IFS PHANTOM
        auto choosePipeline=[&](int mode)->VkPipeline {
            switch(mode){
                case 1:return solidWallPipeline_;
                case 2:return matrixRainPipeline_;
                case 3:return heppGlowPipeline_;
                case 4:return phiFieldPipeline_;
                case 5:return mandelbrotZoomPipeline_;
                default:return liquidRoomPipeline_;
            }
        };
        // Only verified/tracked wall planes receive a room projection.
        // The synthetic development quad remains a fallback until the native
        // Scene query provider populates EnvironmentSurfaceRegistry.
        bool wallRendered=false;
        struct WallCandidate { const voxen::scene::EnvironmentSurface* surface; float distanceSquared; };
        std::vector<WallCandidate> candidates;
        const auto& camera=xrView.pose.position;
        std::vector<const voxen::scene::EnvironmentSurface*> horizontalSurfaces;
        for(const auto& surface:environmentSurfaces_.surfaces()) {
            if(!surface.tracked || surface.halfExtents[0]<=0.05f ||
               surface.halfExtents[1]<=0.05f)continue;
            if(surface.classification==voxen::scene::SurfaceClass::Floor ||
               surface.classification==voxen::scene::SurfaceClass::Ceiling) {
                if(includeFloorCeiling)horizontalSurfaces.push_back(&surface);
                continue;
            }
            if(surface.classification!=voxen::scene::SurfaceClass::Wall)continue;
            const float dx=surface.center[0]-camera.x;
            const float dy=surface.center[1]-camera.y;
            const float dz=surface.center[2]-camera.z;
            candidates.push_back({&surface,dx*dx+dy*dy+dz*dz});
        }
        // Limit fragment-shader fill cost. Same stable ordering in both eyes.
        std::sort(candidates.begin(),candidates.end(),[](const auto& a,const auto& b) {
            if(a.distanceSquared!=b.distanceSquared)return a.distanceSquared<b.distanceSquared;
            return a.surface->id<b.surface->id;
        });
        const size_t budget=std::min(candidates.size(),static_cast<size_t>(maxLiquidWalls));
        // Floor and ceiling are additional planes, not counted against
        // Max walls. Keep each category to one nearest anchor to bound GPU fill.
        std::vector<const voxen::scene::EnvironmentSurface*> selected;
        selected.reserve(budget+2);
        for(size_t i=0;i<budget;i++)selected.push_back(candidates[i].surface);
        for(auto kind:{voxen::scene::SurfaceClass::Floor,voxen::scene::SurfaceClass::Ceiling}) {
            const voxen::scene::EnvironmentSurface* closest=nullptr;
            float best=1e30f;
            for(const auto* surface:horizontalSurfaces) {
                if(surface->classification!=kind)continue;
                const float dx=surface->center[0]-camera.x;
                const float dy=surface->center[1]-camera.y;
                const float dz=surface->center[2]-camera.z;
                const float distance=dx*dx+dy*dy+dz*dz;
                if(distance<best){best=distance;closest=surface;}
            }
            if(closest)selected.push_back(closest);
        }
        for(const auto* surface:selected) {
            int activeMode=wallShaderMode;
            if(cycleWallShaders){
                auto it=std::lower_bound(shaderIds.begin(),shaderIds.end(),surface->id);
                const auto rank=static_cast<int>(it-shaderIds.begin());
                activeMode=demoModes[(rank+cycleStep)%5];
            }
            vkCmdBindPipeline(commandBuffer_,VK_PIPELINE_BIND_POINT_GRAPHICS,choosePipeline(activeMode));
            const Mat4 wallMvp=multiply(projView,surfaceModel(*surface));
            ProceduralPushConstants liquid{wallMvp,{seconds,globalSpeed,1.0F,liquidOpacity}};
            if(activeMode==1) {
                liquid.parameters[0]=wallColor[0];
                liquid.parameters[1]=wallColor[1];
                liquid.parameters[2]=wallColor[2];
                liquid.parameters[3]=solidOpacity;
            } else if(activeMode==2) {
                liquid.parameters[3]=liquidOpacity;
            }
            vkCmdPushConstants(commandBuffer_,pipelineLayout_,
                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                0,sizeof(liquid),&liquid);
            vkCmdDraw(commandBuffer_,6,1,0,0);
            wallRendered=true;
        }
        if(!wallRendered && !cycleWallShaders && wallShaderMode==0) {
            vkCmdBindPipeline(commandBuffer_,VK_PIPELINE_BIND_POINT_GRAPHICS,liquidRoomPipeline_);
            const Mat4 planeMvp=multiply(projView,
                modelMatrix(0.0F,-0.35F,0.65F,-2.25F,0.0F));
            ProceduralPushConstants liquid{planeMvp,{seconds,globalSpeed,1.0F,liquidOpacity}};
            vkCmdPushConstants(commandBuffer_,pipelineLayout_,
                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                0,sizeof(liquid),&liquid);
            vkCmdDraw(commandBuffer_,6,1,0,0);
        }
    }
#ifdef __ANDROID__
    static bool wireframeStateLogged=false;
    if(showSceneMesh && !wireframeStateLogged) {
        __android_log_print(ANDROID_LOG_INFO,"VoxenScene",
            "Wireframe draw requested: vertices=%u pipeline=%d buffer=%d",
            meshWireVertexCount_,meshWirePipeline_!=VK_NULL_HANDLE,
            meshWireBuffer_!=VK_NULL_HANDLE);
        wireframeStateLogged=true;
    }
#endif
    if(showSceneMesh && meshWireVertexCount_>0 && meshWireBuffer_!=VK_NULL_HANDLE) {
        vkCmdBindPipeline(commandBuffer_,VK_PIPELINE_BIND_POINT_GRAPHICS,meshWirePipeline_);
        const VkDeviceSize offset=0;
        vkCmdBindVertexBuffers(commandBuffer_,0,1,&meshWireBuffer_,&offset);
        const auto mvp=multiply(projView,meshPoseMatrix(meshWirePose_));
        ProceduralPushConstants constants{mvp,{0.f,0.f,0.f,0.f}};
        vkCmdPushConstants(commandBuffer_,pipelineLayout_,
            VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT,0,sizeof(constants),&constants);
        vkCmdDraw(commandBuffer_,meshWireVertexCount_,1,0,0);
    }
    if(voxenPlasmaPipeline_!=VK_NULL_HANDLE && voxenSpheresPipeline_!=VK_NULL_HANDLE &&
       voxenMandelbulbPipeline_!=VK_NULL_HANDLE && voxenAttractorPipeline_!=VK_NULL_HANDLE){
        auto paintPlasma=[&](const PlasmaVoxen& v){
            vkCmdBindPipeline(commandBuffer_,VK_PIPELINE_BIND_POINT_GRAPHICS,
                v.preset==1 ? voxenSpheresPipeline_ :
                v.preset==2 ? voxenMandelbulbPipeline_ :
                v.preset==3 ? voxenAttractorPipeline_ : voxenPlasmaPipeline_);
            const Mat4 mvp=multiply(projView,modelMatrix(
                v.position[0],v.position[1],v.scale,v.position[2],0.f));
            ProceduralPushConstants pc{mvp,{seconds,v.speed,v.hue,v.density}};
            vkCmdPushConstants(commandBuffer_,pipelineLayout_,
                VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT,0,sizeof(pc),&pc);
            vkCmdDraw(commandBuffer_,v.preset==3 ? 24*18*2 : 6,1,0,0);
        };
        for(const auto& v:voxens_)paintPlasma(v);
#ifdef VOXEN_WITH_IMGUI
        if(imguiCarousel_.voxenPaintEnabled && controllerTracked_[1] && !panelAimed_){
            const auto& pose=controllerPoses_[1];
            const auto& q=pose.orientation;
            const std::array<float,3> dir{
                -2.f*(q.x*q.z+q.w*q.y),
                -2.f*(q.y*q.z-q.w*q.x),
                -(1.f-2.f*(q.x*q.x+q.y*q.y))};
            PlasmaVoxen tip;
            tip.preset=imguiCarousel_.voxenPreset;
            tip.position={pose.position.x+0.17f*dir[0],
                          pose.position.y+0.17f*dir[1],
                          pose.position.z+0.17f*dir[2]};
            tip.scale=imguiCarousel_.voxenScale;
            tip.hue=imguiCarousel_.voxenHue;
            tip.density=imguiCarousel_.voxenDensity;
            tip.speed=imguiCarousel_.voxenSpeed;
            paintPlasma(tip);
        }
#endif
    }
#ifdef VOXEN_WITH_IMGUI
    // Minimal native eight-page carousel scaffold, left-hand anchored.
    if (controllerTracked_[0] && imguiSurface_.ready() && imguiPanelPipeline_ != VK_NULL_HANDLE) {
        const auto& cp=controllerPoses_[0].position;
        // Keep the panel at a readable distance from both eyes, rather than
        // having the left controller push it through the headset near plane.
        const auto& eye=xrView.pose.position;
        float px=cp.x, py=cp.y+0.17f, pz=cp.z;
        const float dx=px-eye.x, dz=pz-eye.z;
        const float distance=std::sqrt(dx*dx+dz*dz);
        if (distance<0.55f) {
            // Eye/controller coincidence: use the tracked eye's forward direction.
            const auto& q=xrView.pose.orientation;
            const float fx=-2.0f*(q.x*q.z+q.w*q.y);
            const float fz=-(1.0f-2.0f*(q.x*q.x+q.y*q.y));
            const float inv=distance>0.05f ? 1.0f/distance : 0.0f;
            px=eye.x+(inv>0.0f?dx*inv:fx)*0.70f;
            pz=eye.z+(inv>0.0f?dz*inv:fz)*0.70f;
        }
        // Normal faces the eyes. 22 cm high, with base near the controller.
        const float yaw=std::atan2(eye.x-px,eye.z-pz);
        Mat4 menuModel=modelMatrix(px,py,0.22f,pz,yaw);
        menuModel.m[5]*=(672.f/512.f);
        const Mat4 menuMvp=multiply(projView,menuModel);
        ProceduralPushConstants menu{menuMvp,{seconds,1.0f,1.0f,float(nativeMenuPage_)}};
        vkCmdBindPipeline(commandBuffer_,VK_PIPELINE_BIND_POINT_GRAPHICS,imguiPanelPipeline_);
        VkDescriptorSet textureSet=imguiSurface_.descriptor();
        vkCmdBindDescriptorSets(commandBuffer_,VK_PIPELINE_BIND_POINT_GRAPHICS,
            imguiPanelPipelineLayout_,0,1,&textureSet,0,nullptr);
        vkCmdPushConstants(commandBuffer_,imguiPanelPipelineLayout_,
            VK_SHADER_STAGE_VERTEX_BIT,
            0,sizeof(menu),&menu);
        vkCmdDraw(commandBuffer_,6,1,0,0);
    }
#endif
    // Ray is shown only on the right controller when it points at the UI.
#ifdef VOXEN_WITH_IMGUI
    if(controllerTracked_[1] && panelAimed_){
        vkCmdBindPipeline(commandBuffer_,VK_PIPELINE_BIND_POINT_GRAPHICS,laserPipeline_);
        const float length=std::max(0.05f,panelHitDistance_);
        const Mat4 beamMvp=multiply(projView,laserMatrix(controllerPoses_[1],length));
        ProceduralPushConstants constants{beamMvp,{seconds,1.0F,1.0F,0.0F}};
        vkCmdPushConstants(commandBuffer_,pipelineLayout_,
            VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT,0,sizeof(constants),&constants);
        vkCmdDraw(commandBuffer_,36,1,0,0);
    }
#endif
    vkCmdEndRenderPass(commandBuffer_);
    if (gpuTimestampPool_ != VK_NULL_HANDLE)
        vkCmdWriteTimestamp(commandBuffer_, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, gpuTimestampPool_, 1);
    checkVk(vkEndCommandBuffer(commandBuffer_), "vkEndCommandBuffer");
    VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO}; submit.commandBufferCount=1; submit.pCommandBuffers=&commandBuffer_;
    checkVk(vkQueueSubmit(queue_,1,&submit,fence_), "vkQueueSubmit");
    checkVk(vkWaitForFences(device_,1,&fence_,VK_TRUE,UINT64_MAX), "vkWaitForFences");
    if (gpuTimestampPool_ != VK_NULL_HANDLE) {
        uint64_t ts[2]{};
        if (vkGetQueryPoolResults(device_,gpuTimestampPool_,0,2,sizeof(ts),ts,sizeof(uint64_t),
                VK_QUERY_RESULT_64_BIT) == VK_SUCCESS && ts[1] >= ts[0]) {
            const float ms = static_cast<float>(ts[1]-ts[0])*timestampPeriodNs_*1e-6f;
            lastGpuMs_ = completedGpuSamples_++ == 0 ? ms : lastGpuMs_*0.9f+ms*0.1f;
        }
    }
}

void XrSwapchainRenderer::render(
    const std::array<XrView, 2>& views,
    std::array<XrCompositionLayerProjectionView, 2>& projectionViews) {
#ifdef VOXEN_WITH_IMGUI
    if (ImGui::GetCurrentContext() != nullptr) {
        const auto now=std::chrono::steady_clock::now();
        const float dt=lastUiFrame_.time_since_epoch().count() == 0
            ? 1.f/72.f
            : std::clamp(std::chrono::duration<float>(now-lastUiFrame_).count(),1.f/240.f,0.1f);
        lastUiFrame_=now;
        auto& io=ImGui::GetIO();
        io.DeltaTime=dt;
        // Project the right-hand aim ray to the same left-controller panel
        // transform used by the stereo pass.
        bool aiming=false;
        float cursorX=0.f,cursorY=0.f;
        if(controllerTracked_[0] && controllerTracked_[1]) {
            const auto& cp=controllerPoses_[0].position;
            const auto& eye=views[0].pose.position;
            float px=cp.x,py=cp.y+0.29f,pz=cp.z;
            const float dx=px-eye.x,dz=pz-eye.z;
            const float distance=std::sqrt(dx*dx+dz*dz);
            if(distance<0.70f) {
                const auto& q=views[0].pose.orientation;
                const float fx=-2.f*(q.x*q.z+q.w*q.y);
                const float fz=-(1.f-2.f*(q.x*q.x+q.y*q.y));
                const float inv=distance>0.05f?1.f/distance:0.f;
                px=eye.x+(inv>0?dx*inv:fx)*0.70f;
                pz=eye.z+(inv>0?dz*inv:fz)*0.70f;
            }
            const float yaw=std::atan2(eye.x-px,eye.z-pz);
            XrPosef panel{};
            panel.position={px,py,pz};
            panel.orientation={0.f,std::sin(yaw*0.5f),0.f,std::cos(yaw*0.5f)};
            auto hit=voxen::ui::rayToPanel(controllerPoses_[1],panel,0.44f,0.44f*672.f/512.f);
            if(hit.valid) {
                aiming=true;
                panelHitDistance_=hit.distance;
                cursorX=hit.u*512.f;cursorY=hit.v*672.f;
                io.AddMousePosEvent(cursorX,cursorY);
            }
        }
        if(!aiming) io.AddMousePosEvent(-FLT_MAX,-FLT_MAX);
        io.AddMouseButtonEvent(0,aiming && uiTrigger_[1]);
        panelAimed_=aiming;
        if (!aiming) panelHitDistance_=0.0f;
        imguiCarousel_.page=nativeMenuPage_;
        imguiCarousel_.gpuMs=lastGpuMs_;
        imguiCarousel_.gpuAvailable=gpuTimingAvailable() && completedGpuSamples_>0;
        ImGui_ImplVulkan_NewFrame();
        ImGui::NewFrame();
        imguiCarousel_.voxenCount=static_cast<int>(voxens_.size());
        imguiCarousel_.musicReady=ambientMusic_.ready();
        imguiCarousel_.musicFiles=mp3Library_.titles();
        imguiCarousel_.musicError=mp3Library_.error();
        voxen::ui::drawCarousel(imguiCarousel_);
        if(imguiCarousel_.voxenClear){
            voxens_.clear();
            lastVoxenValid_=false;
            imguiCarousel_.voxenClear=false;
        }
        if(imguiCarousel_.musicRescan){
            imguiCarousel_.musicRescan=false;
            mp3Library_.shutdown();
            mp3Library_.refresh();
            imguiCarousel_.musicFile=0;
        }
        const bool procedural=imguiCarousel_.musicSource==0;
        ambientMusic_.configure(imguiCarousel_.musicTrack,
            procedural&&imguiCarousel_.musicPlaying,imguiCarousel_.musicVolume);
        mp3Library_.update(procedural?-1:imguiCarousel_.musicFile,
            !procedural&&imguiCarousel_.musicPlaying,imguiCarousel_.musicVolume);
        // Visible cursor proves that ray → panel UV mapping reaches ImGui.
        if(aiming) {
            ImGui::GetForegroundDrawList()->AddCircleFilled(
                ImVec2(cursorX,cursorY),7.f,IM_COL32(100,255,150,255));
        }
        ImGui::Render();
        nativeMenuPage_=imguiCarousel_.page;
        // ImDrawData is now available for a future offscreen VkImage renderer.
        // For now the old phosphor GLSL panel is still rendered below.
    }
#endif
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

    // Throttled diagnostic while the native Vulkan HUD is under construction.
    // GPU timestamp is measured per eye; this value is the last eye rendered.
    static auto lastStatsLog = std::chrono::steady_clock::now();
    const auto now = std::chrono::steady_clock::now();
    if (now - lastStatsLog >= std::chrono::seconds(2)) {
        lastStatsLog = now;
#ifdef __ANDROID__
        if (gpuTimestampPool_ != VK_NULL_HANDLE && completedGpuSamples_ > 0)
            __android_log_print(ANDROID_LOG_INFO,"VoxenPerf","GPU last-eye %.2f ms",lastGpuMs_);
        else
            __android_log_print(ANDROID_LOG_INFO,"VoxenPerf","GPU N/A (timestamps unsupported)");
#else
        if (gpuTimestampPool_ != VK_NULL_HANDLE && completedGpuSamples_ > 0)
            std::printf("[VoxenPerf] GPU last-eye %.2f ms\\n",lastGpuMs_);
        else
            std::printf("[VoxenPerf] GPU N/A (timestamps unsupported)\\n");
#endif
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
    mp3Library_.shutdown();
    ambientMusic_.stop();
    if (device_ != VK_NULL_HANDLE) vkDeviceWaitIdle(device_);
#ifdef VOXEN_WITH_IMGUI
    imguiSurface_.shutdown();
    if (imguiContextOwned_ && ImGui::GetCurrentContext() != nullptr) {
        ImGui::DestroyContext();
        imguiContextOwned_=false;
    }
#endif
    if (device_ != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(device_);
    }

    for (auto& eye : eyes_) {
        for (auto fb : eye.framebuffers) if (fb != VK_NULL_HANDLE) vkDestroyFramebuffer(device_, fb, nullptr);
        for (auto view : eye.views) if (view != VK_NULL_HANDLE) vkDestroyImageView(device_, view, nullptr);
        eye.framebuffers.clear(); eye.views.clear();
        if (eye.depthView != VK_NULL_HANDLE) { vkDestroyImageView(device_, eye.depthView, nullptr); eye.depthView = VK_NULL_HANDLE; }
        if (eye.depthImage != VK_NULL_HANDLE) { vkDestroyImage(device_, eye.depthImage, nullptr); eye.depthImage = VK_NULL_HANDLE; }
        if (eye.depthMemory != VK_NULL_HANDLE) { vkFreeMemory(device_, eye.depthMemory, nullptr); eye.depthMemory = VK_NULL_HANDLE; }
        if (eye.handle != XR_NULL_HANDLE) {
            xrDestroySwapchain(eye.handle);
            eye.handle = XR_NULL_HANDLE;
        }
        eye.images.clear();
        eye.layouts.clear();
    }

    if(voxenPlasmaPipeline_!=VK_NULL_HANDLE){vkDestroyPipeline(device_,voxenPlasmaPipeline_,nullptr);voxenPlasmaPipeline_=VK_NULL_HANDLE;}
    if(voxenSpheresPipeline_!=VK_NULL_HANDLE){vkDestroyPipeline(device_,voxenSpheresPipeline_,nullptr);voxenSpheresPipeline_=VK_NULL_HANDLE;}
    if(voxenMandelbulbPipeline_!=VK_NULL_HANDLE){vkDestroyPipeline(device_,voxenMandelbulbPipeline_,nullptr);voxenMandelbulbPipeline_=VK_NULL_HANDLE;}
    if(voxenAttractorPipeline_!=VK_NULL_HANDLE){vkDestroyPipeline(device_,voxenAttractorPipeline_,nullptr);voxenAttractorPipeline_=VK_NULL_HANDLE;}
    voxens_.clear();
    if (laserPipeline_ != VK_NULL_HANDLE) { vkDestroyPipeline(device_, laserPipeline_, nullptr); laserPipeline_ = VK_NULL_HANDLE; }
    if (liquidRoomPipeline_ != VK_NULL_HANDLE) { vkDestroyPipeline(device_,liquidRoomPipeline_,nullptr); liquidRoomPipeline_=VK_NULL_HANDLE; }
    if (solidWallPipeline_ != VK_NULL_HANDLE) { vkDestroyPipeline(device_,solidWallPipeline_,nullptr); solidWallPipeline_=VK_NULL_HANDLE; }
    if (meshWirePipeline_ != VK_NULL_HANDLE) { vkDestroyPipeline(device_,meshWirePipeline_,nullptr); meshWirePipeline_=VK_NULL_HANDLE; }
    if(meshDepthPipeline_!=VK_NULL_HANDLE){vkDestroyPipeline(device_,meshDepthPipeline_,nullptr);meshDepthPipeline_=VK_NULL_HANDLE;}
    if(matrixRainPipeline_!=VK_NULL_HANDLE){vkDestroyPipeline(device_,matrixRainPipeline_,nullptr);matrixRainPipeline_=VK_NULL_HANDLE;}
    if(heppGlowPipeline_!=VK_NULL_HANDLE){vkDestroyPipeline(device_,heppGlowPipeline_,nullptr);heppGlowPipeline_=VK_NULL_HANDLE;}
    if(phiFieldPipeline_!=VK_NULL_HANDLE){vkDestroyPipeline(device_,phiFieldPipeline_,nullptr);phiFieldPipeline_=VK_NULL_HANDLE;}
    if(mandelbrotZoomPipeline_!=VK_NULL_HANDLE){vkDestroyPipeline(device_,mandelbrotZoomPipeline_,nullptr);mandelbrotZoomPipeline_=VK_NULL_HANDLE;}
    if(meshDepthBuffer_!=VK_NULL_HANDLE){vkDestroyBuffer(device_,meshDepthBuffer_,nullptr);meshDepthBuffer_=VK_NULL_HANDLE;}
    if(meshDepthMemory_!=VK_NULL_HANDLE){vkFreeMemory(device_,meshDepthMemory_,nullptr);meshDepthMemory_=VK_NULL_HANDLE;}
    meshDepthVertexCount_=0;
    if(meshWireBuffer_!=VK_NULL_HANDLE){vkDestroyBuffer(device_,meshWireBuffer_,nullptr);meshWireBuffer_=VK_NULL_HANDLE;}
    if(meshWireMemory_!=VK_NULL_HANDLE){vkFreeMemory(device_,meshWireMemory_,nullptr);meshWireMemory_=VK_NULL_HANDLE;}
    meshWireVertexCount_=0;
    if (carouselPipeline_ != VK_NULL_HANDLE) { vkDestroyPipeline(device_,carouselPipeline_,nullptr); carouselPipeline_=VK_NULL_HANDLE; }
#ifdef VOXEN_WITH_IMGUI
    if (imguiPanelPipeline_ != VK_NULL_HANDLE) {vkDestroyPipeline(device_,imguiPanelPipeline_,nullptr);imguiPanelPipeline_=VK_NULL_HANDLE;}
    if (imguiPanelPipelineLayout_ != VK_NULL_HANDLE) {vkDestroyPipelineLayout(device_,imguiPanelPipelineLayout_,nullptr);imguiPanelPipelineLayout_=VK_NULL_HANDLE;}
#endif
    if (pipelineLayout_ != VK_NULL_HANDLE) { vkDestroyPipelineLayout(device_, pipelineLayout_, nullptr); pipelineLayout_ = VK_NULL_HANDLE; }
    if (renderPass_ != VK_NULL_HANDLE) { vkDestroyRenderPass(device_, renderPass_, nullptr); renderPass_ = VK_NULL_HANDLE; }

    if (gpuTimestampPool_ != VK_NULL_HANDLE) {
        vkDestroyQueryPool(device_, gpuTimestampPool_, nullptr);
        gpuTimestampPool_ = VK_NULL_HANDLE;
    }
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
    physicalDevice_ = VK_NULL_HANDLE;
    depthFormat_ = VK_FORMAT_UNDEFINED;
    session_ = XR_NULL_HANDLE;
}

} // namespace voxen::render
