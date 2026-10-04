#include <voxen/xr/XrSession.hpp>

#include <jni.h>
#include <openxr/openxr_platform.h>

#include <stdexcept>
#include <string>

namespace voxen::xr {
namespace {

void checkXr(XrResult result, const char* operation) {
    if (XR_FAILED(result)) {
        throw std::runtime_error(std::string(operation) + " failed with XrResult " + std::to_string(result));
    }
}

} // namespace

XrSession::~XrSession() {
    shutdown();
}

void XrSession::initialize(
    XrInstance instance,
    XrSystemId systemId,
    VkInstance vkInstance,
    VkPhysicalDevice physicalDevice,
    VkDevice device,
    uint32_t queueFamilyIndex) {
    if (session_ != XR_NULL_HANDLE) {
        throw std::logic_error("OpenXR session is already initialized");
    }

    instance_ = instance;

    XrGraphicsBindingVulkan2KHR binding{XR_TYPE_GRAPHICS_BINDING_VULKAN2_KHR};
    binding.instance = vkInstance;
    binding.physicalDevice = physicalDevice;
    binding.device = device;
    binding.queueFamilyIndex = queueFamilyIndex;
    binding.queueIndex = 0;

    XrSessionCreateInfo createInfo{XR_TYPE_SESSION_CREATE_INFO};
    createInfo.next = &binding;
    createInfo.systemId = systemId;
    checkXr(xrCreateSession(instance_, &createInfo, &session_), "xrCreateSession");

    XrReferenceSpaceCreateInfo spaceInfo{XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
    spaceInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
    spaceInfo.poseInReferenceSpace.orientation.w = 1.0F;
    checkXr(xrCreateReferenceSpace(session_, &spaceInfo, &localSpace_), "xrCreateReferenceSpace(LOCAL)");
}

bool XrSession::pollEvents() {
    XrEventDataBuffer event{XR_TYPE_EVENT_DATA_BUFFER};

    while (true) {
        const XrResult result = xrPollEvent(instance_, &event);
        if (result == XR_EVENT_UNAVAILABLE) {
            break;
        }
        checkXr(result, "xrPollEvent");

        switch (event.type) {
        case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED:
            handleSessionStateChanged(*reinterpret_cast<const XrEventDataSessionStateChanged*>(&event));
            break;
        case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING:
            return false;
        default:
            break;
        }

        event = {XR_TYPE_EVENT_DATA_BUFFER};
    }

    return state_ != XR_SESSION_STATE_EXITING && state_ != XR_SESSION_STATE_LOSS_PENDING;
}

void XrSession::handleSessionStateChanged(const XrEventDataSessionStateChanged& event) {
    state_ = event.state;

    if (state_ == XR_SESSION_STATE_READY && !running_) {
        XrSessionBeginInfo beginInfo{XR_TYPE_SESSION_BEGIN_INFO};
        beginInfo.primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
        checkXr(xrBeginSession(session_, &beginInfo), "xrBeginSession");
        running_ = true;
    } else if (state_ == XR_SESSION_STATE_STOPPING && running_) {
        checkXr(xrEndSession(session_), "xrEndSession");
        running_ = false;
    }
}

void XrSession::frame() {
    if (!running_) {
        return;
    }

    XrFrameWaitInfo waitInfo{XR_TYPE_FRAME_WAIT_INFO};
    XrFrameState frameState{XR_TYPE_FRAME_STATE};
    checkXr(xrWaitFrame(session_, &waitInfo, &frameState), "xrWaitFrame");

    XrFrameBeginInfo beginInfo{XR_TYPE_FRAME_BEGIN_INFO};
    checkXr(xrBeginFrame(session_, &beginInfo), "xrBeginFrame");

    XrFrameEndInfo endInfo{XR_TYPE_FRAME_END_INFO};
    endInfo.displayTime = frameState.predictedDisplayTime;
    endInfo.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
    endInfo.layerCount = 0;
    endInfo.layers = nullptr;
    checkXr(xrEndFrame(session_, &endInfo), "xrEndFrame");
}

void XrSession::shutdown() noexcept {
    if (localSpace_ != XR_NULL_HANDLE) {
        xrDestroySpace(localSpace_);
        localSpace_ = XR_NULL_HANDLE;
    }

    if (session_ != XR_NULL_HANDLE) {
        if (running_) {
            xrEndSession(session_);
            running_ = false;
        }
        xrDestroySession(session_);
        session_ = XR_NULL_HANDLE;
    }

    state_ = XR_SESSION_STATE_UNKNOWN;
    instance_ = XR_NULL_HANDLE;
}

} // namespace voxen::xr
