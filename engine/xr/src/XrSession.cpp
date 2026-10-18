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
#include <cstring>
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
    bool enablePassthrough,
    bool enableScene) {
    if (session_ != XR_NULL_HANDLE) {
        throw std::logic_error("OpenXR session is already initialized");
    }

    instance_ = instance;
    enablePassthrough_ = enablePassthrough;
    enableScene_ = enableScene;

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
    initializeActions();
    if(enableScene_) metaScene_.initialize(instance_,session_);
    if (enablePassthrough_) {
        initializePassthrough();
#ifdef __ANDROID__
        __android_log_print(ANDROID_LOG_INFO, "Voxen", "Passthrough layer created");
#else
        std::printf("[Voxen] Passthrough layer created\n");
#endif
    }
}

void XrSession::initializeActions() {
    checkXr(xrStringToPath(instance_, "/user/hand/left", &handPaths_[0]), "xrStringToPath(left)");
    checkXr(xrStringToPath(instance_, "/user/hand/right", &handPaths_[1]), "xrStringToPath(right)");

    XrActionSetCreateInfo setInfo{XR_TYPE_ACTION_SET_CREATE_INFO};
    std::strncpy(setInfo.actionSetName, "voxen_interaction", XR_MAX_ACTION_SET_NAME_SIZE - 1);
    std::strncpy(setInfo.localizedActionSetName, "Voxen Interaction", XR_MAX_LOCALIZED_ACTION_SET_NAME_SIZE - 1);
    setInfo.priority = 0;
    checkXr(xrCreateActionSet(instance_, &setInfo, &actionSet_), "xrCreateActionSet");

    XrActionCreateInfo actionInfo{XR_TYPE_ACTION_CREATE_INFO};
    actionInfo.actionType = XR_ACTION_TYPE_BOOLEAN_INPUT;
    std::strncpy(actionInfo.actionName, "select", XR_MAX_ACTION_NAME_SIZE - 1);
    std::strncpy(actionInfo.localizedActionName, "Select artwork", XR_MAX_LOCALIZED_ACTION_NAME_SIZE - 1);
    actionInfo.countSubactionPaths = static_cast<uint32_t>(handPaths_.size());
    actionInfo.subactionPaths = handPaths_.data();
    checkXr(xrCreateAction(actionSet_, &actionInfo, &selectAction_), "xrCreateAction(select)");

    XrActionCreateInfo menuInfo{XR_TYPE_ACTION_CREATE_INFO};
    menuInfo.actionType=XR_ACTION_TYPE_BOOLEAN_INPUT;
    std::strncpy(menuInfo.actionName,"toggle_ui",XR_MAX_ACTION_NAME_SIZE-1);
    std::strncpy(menuInfo.localizedActionName,"Show or hide Voxen menu",XR_MAX_LOCALIZED_ACTION_NAME_SIZE-1);
    checkXr(xrCreateAction(actionSet_,&menuInfo,&leftMenuAction_),"xrCreateAction(toggle_ui)");

    XrActionCreateInfo aimInfo{XR_TYPE_ACTION_CREATE_INFO};
    aimInfo.actionType = XR_ACTION_TYPE_POSE_INPUT;
    std::strncpy(aimInfo.actionName, "aim_pose", XR_MAX_ACTION_NAME_SIZE - 1);
    std::strncpy(aimInfo.localizedActionName, "Controller Aim", XR_MAX_LOCALIZED_ACTION_NAME_SIZE - 1);
    aimInfo.countSubactionPaths = static_cast<uint32_t>(handPaths_.size());
    aimInfo.subactionPaths = handPaths_.data();
    checkXr(xrCreateAction(actionSet_, &aimInfo, &aimAction_), "xrCreateAction(aim)");

    XrActionCreateInfo stickInfo{XR_TYPE_ACTION_CREATE_INFO};
    stickInfo.actionType = XR_ACTION_TYPE_VECTOR2F_INPUT;
    std::strncpy(stickInfo.actionName, "thumbstick", XR_MAX_ACTION_NAME_SIZE - 1);
    std::strncpy(stickInfo.localizedActionName, "Artwork Translation", XR_MAX_LOCALIZED_ACTION_NAME_SIZE - 1);
    stickInfo.countSubactionPaths = static_cast<uint32_t>(handPaths_.size());
    stickInfo.subactionPaths = handPaths_.data();
    checkXr(xrCreateAction(actionSet_, &stickInfo, &thumbstickAction_), "xrCreateAction(thumbstick)");

    XrPath profile = XR_NULL_PATH;
    XrPath leftTrigger = XR_NULL_PATH;
    XrPath leftMenu = XR_NULL_PATH;
    XrPath rightTrigger = XR_NULL_PATH;
    XrPath leftAim = XR_NULL_PATH;
    XrPath rightAim = XR_NULL_PATH;
    XrPath leftStick = XR_NULL_PATH;
    XrPath rightStick = XR_NULL_PATH;
    checkXr(xrStringToPath(instance_, "/interaction_profiles/oculus/touch_controller", &profile), "xrStringToPath(touch)");
    checkXr(xrStringToPath(instance_, "/user/hand/left/input/trigger/value", &leftTrigger), "xrStringToPath(left trigger)");
    checkXr(xrStringToPath(instance_, "/user/hand/left/input/menu/click", &leftMenu), "xrStringToPath(left menu)");
    checkXr(xrStringToPath(instance_, "/user/hand/right/input/trigger/value", &rightTrigger), "xrStringToPath(right trigger)");

    checkXr(xrStringToPath(instance_, "/user/hand/left/input/aim/pose", &leftAim), "xrStringToPath(left aim)");
    checkXr(xrStringToPath(instance_, "/user/hand/right/input/aim/pose", &rightAim), "xrStringToPath(right aim)");

    checkXr(xrStringToPath(instance_, "/user/hand/left/input/thumbstick", &leftStick), "xrStringToPath(left thumbstick)");
    checkXr(xrStringToPath(instance_, "/user/hand/right/input/thumbstick", &rightStick), "xrStringToPath(right thumbstick)");

    // Boolean actions may bind to floating trigger inputs using OpenXR's threshold mapping.
    const XrActionSuggestedBinding bindings[] = {
        {selectAction_, leftTrigger},
        {leftMenuAction_, leftMenu},
        {selectAction_, rightTrigger},
        {aimAction_, leftAim},
        {aimAction_, rightAim},
        {thumbstickAction_, leftStick},
        {thumbstickAction_, rightStick},
    };
    XrInteractionProfileSuggestedBinding suggested{XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};
    suggested.interactionProfile = profile;
    suggested.countSuggestedBindings = static_cast<uint32_t>(sizeof(bindings)/sizeof(bindings[0]));
    suggested.suggestedBindings = bindings;
    checkXr(xrSuggestInteractionProfileBindings(instance_, &suggested), "xrSuggestInteractionProfileBindings");

    XrSessionActionSetsAttachInfo attach{XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO};
    attach.countActionSets = 1;
    attach.actionSets = &actionSet_;
    checkXr(xrAttachSessionActionSets(session_, &attach), "xrAttachSessionActionSets");
    XrActionStateGetInfo menuGet{XR_TYPE_ACTION_STATE_GET_INFO};
    menuGet.action=leftMenuAction_;
    XrActionStateBoolean menuState{XR_TYPE_ACTION_STATE_BOOLEAN};
    checkXr(xrGetActionStateBoolean(session_,&menuGet,&menuState),"xrGetActionStateBoolean(menu)");
    const bool menuNow=menuState.isActive==XR_TRUE && menuState.currentState==XR_TRUE;
    if(menuNow && !menuPressed_)menuTogglePending_=true;
    menuPressed_=menuNow;
    for (std::size_t hand = 0; hand < handPaths_.size(); ++hand) {
        XrActionSpaceCreateInfo space{XR_TYPE_ACTION_SPACE_CREATE_INFO};
        space.action = aimAction_;
        space.subactionPath = handPaths_[hand];
        space.poseInActionSpace.orientation.w = 1.0F;
        checkXr(xrCreateActionSpace(session_, &space, &aimSpaces_[hand]), "xrCreateActionSpace(aim)");
    }
#ifdef __ANDROID__
    __android_log_print(ANDROID_LOG_INFO, "Voxen", "Touch controller actions attached");
#else
    std::printf("[Voxen] Touch controller actions attached\n");
#endif
}

void XrSession::syncActions() {
    XrActiveActionSet active{actionSet_, XR_NULL_PATH};
    XrActionsSyncInfo sync{XR_TYPE_ACTIONS_SYNC_INFO};
    sync.countActiveActionSets = 1;
    sync.activeActionSets = &active;
    checkXr(xrSyncActions(session_, &sync), "xrSyncActions");

    for (std::size_t hand = 0; hand < handPaths_.size(); ++hand) {
        XrActionStateGetInfo get{XR_TYPE_ACTION_STATE_GET_INFO};
        get.action = selectAction_;
        get.subactionPath = handPaths_[hand];
        XrActionStateBoolean state{XR_TYPE_ACTION_STATE_BOOLEAN};
        checkXr(xrGetActionStateBoolean(session_, &get, &state), "xrGetActionStateBoolean(select)");
        const bool pressed = state.isActive == XR_TRUE && state.currentState == XR_TRUE;
        if (pressed && !selectionPressed_[hand]) {
#ifdef __ANDROID__
            __android_log_print(ANDROID_LOG_INFO, "Voxen", "%s trigger selected", hand == 0 ? "Left" : "Right");
#else
            std::printf("[Voxen] %s trigger selected\n", hand == 0 ? "Left" : "Right");
#endif
        }
        selectionPressed_[hand] = pressed;
        get.action = thumbstickAction_;
        XrActionStateVector2f stickState{XR_TYPE_ACTION_STATE_VECTOR2F};
        checkXr(xrGetActionStateVector2f(session_, &get, &stickState), "xrGetActionStateVector2f(thumbstick)");
        thumbstickValues_[hand] = stickState.isActive == XR_TRUE ? stickState.currentState : XrVector2f{};
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

        if(enableScene_) metaScene_.onEvent(reinterpret_cast<const XrEventDataBaseHeader*>(&event));
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
        if(enableScene_) metaScene_.request();
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

    syncActions();
    if(menuTogglePending_){
        renderer.toggleUiVisibility();
        menuTogglePending_=false;
    }
    std::array<XrPosef, 2> aimPoses{};
    std::array<bool, 2> aimTracked{};
    for (std::size_t hand = 0; hand < aimSpaces_.size(); ++hand) {
        XrSpaceLocation location{XR_TYPE_SPACE_LOCATION};
        checkXr(xrLocateSpace(aimSpaces_[hand], localSpace_, frameState.predictedDisplayTime, &location), "xrLocateSpace(aim)");
        constexpr XrSpaceLocationFlags required = XR_SPACE_LOCATION_POSITION_VALID_BIT | XR_SPACE_LOCATION_ORIENTATION_VALID_BIT;
        if ((location.locationFlags & required) == required) {
            aimTracked[hand] = true;
            aimPoses[hand] = location.pose;
        }
    }
    renderer.updateControllerInteraction(aimPoses, aimTracked, selectionPressed_, thumbstickValues_);
    if(enableScene_) {
        metaScene_.refresh(localSpace_,frameState.predictedDisplayTime);
        renderer.setEnvironmentSurfaces(metaScene_.surfaces());
        std::vector<voxen::render::XrSwapchainRenderer::MeshGeometry> meshes;
        for(const auto& mesh:metaScene_.meshes()) {
            if(!mesh.located || mesh.vertices.empty())continue;
            voxen::render::XrSwapchainRenderer::MeshGeometry geometry{};
            geometry.pose=mesh.pose;
            // Copy the geometry only for the first upload; future frames
            // currently share the mesh and refresh its anchor pose.
            geometry.vertices=&mesh.vertices;
            geometry.indices=&mesh.indices;
            meshes.push_back(std::move(geometry));
            break;
        }
#ifdef __ANDROID__
        static bool meshAvailabilityLogged=false;
        if(!meshAvailabilityLogged && !metaScene_.meshes().empty()) {
            const auto& first=metaScene_.meshes().front();
            __android_log_print(ANDROID_LOG_INFO,"VoxenScene",
                "Mesh bridge: captured=%zu located=%d vertices=%zu indices=%zu",
                metaScene_.meshes().size(),first.located?1:0,
                first.vertices.size(),first.indices.size());
            meshAvailabilityLogged=true;
        }
#endif
        if(!meshes.empty())renderer.setSceneMeshes(meshes);
    }

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
    metaScene_.shutdown();
    destroyPassthrough();
    for (auto& space : aimSpaces_) {
        if (space != XR_NULL_HANDLE) xrDestroySpace(space);
        space = XR_NULL_HANDLE;
    }
    if (actionSet_ != XR_NULL_HANDLE) {
        xrDestroyActionSet(actionSet_);
        actionSet_ = XR_NULL_HANDLE;
        selectAction_ = XR_NULL_HANDLE;
        aimAction_ = XR_NULL_HANDLE;
        thumbstickAction_ = XR_NULL_HANDLE;
    }
    selectionPressed_ = {};
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
