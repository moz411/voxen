#include <voxen/xr/XrSession.hpp>
#include <voxen/render/XrSwapchainRenderer.hpp>

#ifdef __ANDROID__
#include <android/log.h>
#include <jni.h>
#else
#include <cstdio>
#endif
#ifdef _WIN32
#include <windows.h>
#include <unknwn.h>
#endif
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
    uint32_t queueFamilyIndex,
    bool enablePassthrough) {
    if (session_ != XR_NULL_HANDLE) {
        throw std::logic_error("OpenXR session is already initialized");
    }

    instance_ = instance;
    enablePassthrough_ = enablePassthrough;

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
    if (enablePassthrough_) {
        initializePassthrough();
#ifdef __ANDROID__
        __android_log_print(ANDROID_LOG_INFO, "Voxen", "Passthrough layer created");
#else
        std::printf("[Voxen] Passthrough layer created\n");
#endif
    }
}

void XrSession::initializePassthrough() {
    auto load = [this](const char* name, PFN_xrVoidFunction* fn) {
        checkXr(xrGetInstanceProcAddr(instance_, name, fn), name);
        if (!*fn) throw std::runtime_error(std::string("Missing OpenXR function: ") + name);
    };
    load("xrCreatePassthroughFB", reinterpret_cast<PFN_xrVoidFunction*>(&createPassthrough_));
    load("xrDestroyPassthroughFB", reinterpret_cast<PFN_xrVoidFunction*>(&destroyPassthrough_));
    load("xrPassthroughStartFB", reinterpret_cast<PFN_xrVoidFunction*>(&startPassthrough_));
    load("xrPassthroughPauseFB", reinterpret_cast<PFN_xrVoidFunction*>(&pausePassthrough_));
    load("xrCreatePassthroughLayerFB", reinterpret_cast<PFN_xrVoidFunction*>(&createLayer_));
    load("xrDestroyPassthroughLayerFB", reinterpret_cast<PFN_xrVoidFunction*>(&destroyLayer_));
    load("xrPassthroughLayerSetStyleFB", reinterpret_cast<PFN_xrVoidFunction*>(&setLayerStyle_));

    XrPassthroughCreateInfoFB createInfo{XR_TYPE_PASSTHROUGH_CREATE_INFO_FB};
    checkXr(createPassthrough_(session_, &createInfo, &passthrough_), "xrCreatePassthroughFB");
    XrPassthroughLayerCreateInfoFB layerInfo{XR_TYPE_PASSTHROUGH_LAYER_CREATE_INFO_FB};
    layerInfo.passthrough = passthrough_;
    layerInfo.purpose = XR_PASSTHROUGH_LAYER_PURPOSE_RECONSTRUCTION_FB;
    layerInfo.flags = XR_PASSTHROUGH_IS_RUNNING_AT_CREATION_BIT_FB;
    checkXr(createLayer_(session_, &layerInfo, &passthroughLayer_), "xrCreatePassthroughLayerFB");

    // The compositor performs native edge highlighting; no camera frame readback.
    XrPassthroughColorMapMonoToRgbaFB palette{XR_TYPE_PASSTHROUGH_COLOR_MAP_MONO_TO_RGBA_FB};
    for (uint32_t i = 0; i < XR_PASSTHROUGH_COLOR_MAP_MONO_SIZE_FB; ++i) {
        const float v = static_cast<float>(i) / 255.0F;
        palette.textureColorMap[i] = {0.015F + 0.06F*v, 0.045F + 0.22F*v, 0.16F + 0.66F*v, 1.0F};
    }
    XrPassthroughStyleFB style{XR_TYPE_PASSTHROUGH_STYLE_FB};
    style.next = &palette;
    style.textureOpacityFactor = 1.0F;
    style.edgeColor = {1.0F, 1.0F, 1.0F, 1.0F};
    checkXr(setLayerStyle_(passthroughLayer_, &style), "xrPassthroughLayerSetStyleFB");
}

void XrSession::destroyPassthrough() noexcept {
    if (passthroughLayer_ != XR_NULL_HANDLE && destroyLayer_) destroyLayer_(passthroughLayer_);
    passthroughLayer_ = XR_NULL_HANDLE;
    if (passthrough_ != XR_NULL_HANDLE && destroyPassthrough_) destroyPassthrough_(passthrough_);
    passthrough_ = XR_NULL_HANDLE;
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

#ifdef __ANDROID__
    __android_log_print(ANDROID_LOG_INFO, "Voxen", "OpenXR session state -> %d", static_cast<int>(state_));
#else
    std::printf("[Voxen] OpenXR session state -> %d\n", static_cast<int>(state_));
#endif

    if (state_ == XR_SESSION_STATE_READY && !running_) {
        XrSessionBeginInfo beginInfo{XR_TYPE_SESSION_BEGIN_INFO};
        beginInfo.primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
        checkXr(xrBeginSession(session_, &beginInfo), "xrBeginSession");
        running_ = true;
        if (passthrough_ != XR_NULL_HANDLE) {
            checkXr(startPassthrough_(passthrough_), "xrPassthroughStartFB");
#ifdef __ANDROID__
            __android_log_print(ANDROID_LOG_INFO, "Voxen", "Passthrough started");
#else
            std::printf("[Voxen] Passthrough started\n");
#endif
        }
#ifdef __ANDROID__
        __android_log_print(ANDROID_LOG_INFO, "Voxen", "OpenXR session begun");
#else
        std::printf("[Voxen] OpenXR session begun\n");
#endif
    } else if (state_ == XR_SESSION_STATE_STOPPING && running_) {
        if (passthrough_ != XR_NULL_HANDLE) checkXr(pausePassthrough_(passthrough_), "xrPassthroughPauseFB");
        checkXr(xrEndSession(session_), "xrEndSession");
        running_ = false;
    }
}

void XrSession::frame(voxen::render::XrSwapchainRenderer& renderer) {
    if (!running_) {
        return;
    }

    XrFrameWaitInfo waitInfo{XR_TYPE_FRAME_WAIT_INFO};
    XrFrameState frameState{XR_TYPE_FRAME_STATE};
    checkXr(xrWaitFrame(session_, &waitInfo, &frameState), "xrWaitFrame");

    XrFrameBeginInfo beginInfo{XR_TYPE_FRAME_BEGIN_INFO};
    checkXr(xrBeginFrame(session_, &beginInfo), "xrBeginFrame");

    XrCompositionLayerProjection layer{XR_TYPE_COMPOSITION_LAYER_PROJECTION};
    std::array<XrCompositionLayerProjectionView, 2> projectionViews{};
    XrCompositionLayerPassthroughFB passthroughComposition{XR_TYPE_COMPOSITION_LAYER_PASSTHROUGH_FB};
    passthroughComposition.layerHandle = passthroughLayer_;
    const XrCompositionLayerBaseHeader* layers[2]{};
    uint32_t layerCount = 0;

    if (frameState.shouldRender == XR_TRUE) {
        std::array<XrView, 2> views = {
            XrView{XR_TYPE_VIEW},
            XrView{XR_TYPE_VIEW},
        };
        XrViewState viewState{XR_TYPE_VIEW_STATE};
        XrViewLocateInfo locateInfo{XR_TYPE_VIEW_LOCATE_INFO};
        locateInfo.viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
        locateInfo.displayTime = frameState.predictedDisplayTime;
        locateInfo.space = localSpace_;

        uint32_t viewCount = 0;
        checkXr(
            xrLocateViews(
                session_,
                &locateInfo,
                &viewState,
                static_cast<uint32_t>(views.size()),
                &viewCount,
                views.data()),
            "xrLocateViews");

        if (viewCount == views.size()) {
            static bool firstLocatedViewsLogged = false;
            if (!firstLocatedViewsLogged) {
#ifdef __ANDROID__
                __android_log_print(ANDROID_LOG_INFO, "Voxen", "Located stereo views; submitting projection layer");
#else
                std::printf("[Voxen] Located stereo views; submitting projection layer\n");
#endif
                firstLocatedViewsLogged = true;
            }
            renderer.render(views, projectionViews);
            layer.space = localSpace_;
            layer.viewCount = static_cast<uint32_t>(projectionViews.size());
            layer.views = projectionViews.data();
            if (passthroughLayer_ != XR_NULL_HANDLE) {
                layers[layerCount++] = reinterpret_cast<const XrCompositionLayerBaseHeader*>(&passthroughComposition);
                layer.layerFlags = XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT;
                static bool loggedComposition = false;
                if (!loggedComposition) {
#ifdef __ANDROID__
                    __android_log_print(ANDROID_LOG_INFO, "Voxen", "Submitting passthrough and transparent projection layers");
#else
                    std::printf("[Voxen] Submitting passthrough and transparent projection layers\n");
#endif
                    loggedComposition = true;
                }
            }
            layers[layerCount++] = reinterpret_cast<const XrCompositionLayerBaseHeader*>(&layer);
        }
    }

    XrFrameEndInfo endInfo{XR_TYPE_FRAME_END_INFO};
    endInfo.displayTime = frameState.predictedDisplayTime;
    endInfo.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
    endInfo.layerCount = layerCount;
    endInfo.layers = layerCount != 0 ? layers : nullptr;
    checkXr(xrEndFrame(session_, &endInfo), "xrEndFrame");
}

void XrSession::shutdown() noexcept {
    destroyPassthrough();
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
