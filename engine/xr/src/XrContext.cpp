#include <voxen/xr/XrContext.hpp>

#ifdef __ANDROID__
#include <jni.h>
#include <android/log.h>
#endif
#include <vulkan/vulkan.h>
#ifdef _WIN32
#include <windows.h>
#include <unknwn.h>
#endif
#include <openxr/openxr_platform.h>

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

namespace voxen::xr {
namespace {

void checkXr(XrResult result, const char* operation) {
    if (XR_FAILED(result)) {
        throw std::runtime_error(std::string(operation) + " failed with XrResult " + std::to_string(result));
    }
}

} // namespace

XrContext::~XrContext() {
    shutdown();
}

#ifdef __ANDROID__
void XrContext::initialize(JavaVM* vm, jobject activity) {
    if (initialized()) {
        throw std::logic_error("OpenXR context is already initialized");
    }
    if (vm == nullptr || activity == nullptr) {
        throw std::invalid_argument("OpenXR Android initialization requires a JavaVM and Activity");
    }

    initializeLoader(vm, activity);
    createInstance();

    try {
        requireExtension(XR_KHR_VULKAN_ENABLE2_EXTENSION_NAME);
        selectSystem();
    } catch (...) {
        shutdown();
        throw;
    }
}

void XrContext::initializeLoader(JavaVM* vm, jobject activity) {
    PFN_xrInitializeLoaderKHR initializeLoader = nullptr;
    checkXr(
        xrGetInstanceProcAddr(
            XR_NULL_HANDLE,
            "xrInitializeLoaderKHR",
            reinterpret_cast<PFN_xrVoidFunction*>(&initializeLoader)),
        "xrGetInstanceProcAddr(xrInitializeLoaderKHR)");

    if (initializeLoader == nullptr) {
        throw std::runtime_error("OpenXR loader does not expose xrInitializeLoaderKHR");
    }

    XrLoaderInitInfoAndroidKHR initInfo{XR_TYPE_LOADER_INIT_INFO_ANDROID_KHR};
    initInfo.applicationVM = vm;
    initInfo.applicationContext = activity;

    checkXr(
        initializeLoader(reinterpret_cast<const XrLoaderInitInfoBaseHeaderKHR*>(&initInfo)),
        "xrInitializeLoaderKHR");
}

#else
void XrContext::initialize() {
    if (initialized()) {
        throw std::logic_error("OpenXR context is already initialized");
    }

    createInstance();
    try {
        requireExtension(XR_KHR_VULKAN_ENABLE2_EXTENSION_NAME);
        selectSystem();
    } catch (...) {
        shutdown();
        throw;
    }
}
#endif

void XrContext::createInstance() {
    requireExtension(XR_KHR_VULKAN_ENABLE2_EXTENSION_NAME);

    std::vector<const char*> extensions{XR_KHR_VULKAN_ENABLE2_EXTENSION_NAME};
    uint32_t count = 0;
    checkXr(xrEnumerateInstanceExtensionProperties(nullptr, 0, &count, nullptr), "enumerate extensions");
    std::vector<XrExtensionProperties> available(count, {XR_TYPE_EXTENSION_PROPERTIES});
    checkXr(xrEnumerateInstanceExtensionProperties(nullptr, count, &count, available.data()), "enumerate extension names");
    passthroughEnabled_ = std::any_of(available.begin(), available.end(), [](const auto& e) {
        return std::strcmp(e.extensionName, XR_FB_PASSTHROUGH_EXTENSION_NAME) == 0;
    });
#ifdef __ANDROID__
    __android_log_print(ANDROID_LOG_INFO, "Voxen", "XR_FB_passthrough available: %s", passthroughEnabled_ ? "yes" : "no");
    if (!passthroughEnabled_) {
        throw std::runtime_error("XR_FB_passthrough unavailable: check com.oculus.feature.PASSTHROUGH manifest flag and Meta runtime");
    }
#else
    std::printf("[Voxen] XR_FB_passthrough available: %s\n", passthroughEnabled_ ? "yes" : "no");
#endif
    if (passthroughEnabled_) extensions.push_back(XR_FB_PASSTHROUGH_EXTENSION_NAME);
    if(std::any_of(available.begin(),available.end(),[](const auto& e){
        return std::strcmp(e.extensionName,XR_EXT_HAND_TRACKING_EXTENSION_NAME)==0;
    }))extensions.push_back(XR_EXT_HAND_TRACKING_EXTENSION_NAME);
    // Scene Model is optional: never prevent startup when spatial data is
    // unavailable or the user has not completed Space Setup.
    const auto has=[&](const char* name) {
        return std::any_of(available.begin(),available.end(),[&](const auto& e) {
            return std::strcmp(e.extensionName,name)==0;
        });
    };
    // One line per capability: an aggregate "unavailable" hides which
    // extension is missing on the installed Quest runtime.
    const std::array<const char*, 8> sceneExtensions = {
        XR_FB_SCENE_EXTENSION_NAME,
        XR_FB_SPATIAL_ENTITY_EXTENSION_NAME,
        XR_FB_SPATIAL_ENTITY_QUERY_EXTENSION_NAME,
        XR_FB_SPATIAL_ENTITY_CONTAINER_EXTENSION_NAME,
        XR_FB_SCENE_CAPTURE_EXTENSION_NAME,
        XR_META_SPATIAL_ENTITY_DISCOVERY_EXTENSION_NAME,
        XR_META_SPATIAL_ENTITY_PERSISTENCE_EXTENSION_NAME,
        XR_META_SPATIAL_ENTITY_MESH_EXTENSION_NAME
    };
    for(const char* name:sceneExtensions) {
#ifdef __ANDROID__
        __android_log_print(ANDROID_LOG_INFO,"VoxenScene",
            "Extension %s: %s",name,has(name)?"yes":"NO");
#else
        std::printf("[VoxenScene] Extension %s: %s\\n",name,has(name)?"yes":"NO");
#endif
    }
    sceneEnabled_=has(XR_FB_SCENE_EXTENSION_NAME) &&
                  has(XR_FB_SPATIAL_ENTITY_EXTENSION_NAME) &&
                  has(XR_FB_SPATIAL_ENTITY_QUERY_EXTENSION_NAME);
    if(sceneEnabled_) {
        if(has(XR_META_SPATIAL_ENTITY_DISCOVERY_EXTENSION_NAME))
            extensions.push_back(XR_META_SPATIAL_ENTITY_DISCOVERY_EXTENSION_NAME);
        extensions.push_back(XR_FB_SPATIAL_ENTITY_EXTENSION_NAME);
        extensions.push_back(XR_FB_SPATIAL_ENTITY_QUERY_EXTENSION_NAME);
        extensions.push_back(XR_FB_SCENE_EXTENSION_NAME);
        if(has(XR_META_SPATIAL_ENTITY_MESH_EXTENSION_NAME))
            extensions.push_back(XR_META_SPATIAL_ENTITY_MESH_EXTENSION_NAME);
    }
#ifdef __ANDROID__
    __android_log_print(ANDROID_LOG_INFO,"VoxenScene",
        "Meta Scene extensions: %s",sceneEnabled_?"enabled":"unavailable");
#else
    std::printf("[VoxenScene] Meta Scene extensions: %s\\n",sceneEnabled_?"enabled":"unavailable");
#endif


    XrInstanceCreateInfo createInfo{XR_TYPE_INSTANCE_CREATE_INFO};
    std::strncpy(createInfo.applicationInfo.applicationName, "Voxen", XR_MAX_APPLICATION_NAME_SIZE - 1);
    createInfo.applicationInfo.applicationVersion = 1;
    std::strncpy(createInfo.applicationInfo.engineName, "Voxen", XR_MAX_ENGINE_NAME_SIZE - 1);
    createInfo.applicationInfo.engineVersion = 1;
    createInfo.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.enabledExtensionNames = extensions.data();

    checkXr(xrCreateInstance(&createInfo, &instance_), "xrCreateInstance");
}

void XrContext::selectSystem() {
    XrSystemGetInfo systemInfo{XR_TYPE_SYSTEM_GET_INFO};
    systemInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
    checkXr(xrGetSystem(instance_, &systemInfo, &systemId_), "xrGetSystem");

    XrSystemProperties properties{XR_TYPE_SYSTEM_PROPERTIES};
    checkXr(xrGetSystemProperties(instance_, systemId_, &properties), "xrGetSystemProperties");
}

void XrContext::requireExtension(const char* extensionName) const {
    uint32_t count = 0;
    checkXr(xrEnumerateInstanceExtensionProperties(nullptr, 0, &count, nullptr),
            "xrEnumerateInstanceExtensionProperties(count)");

    std::vector<XrExtensionProperties> extensions(count, {XR_TYPE_EXTENSION_PROPERTIES});
    checkXr(xrEnumerateInstanceExtensionProperties(nullptr, count, &count, extensions.data()),
            "xrEnumerateInstanceExtensionProperties");

    const auto found = std::any_of(extensions.begin(), extensions.end(), [extensionName](const auto& extension) {
        return std::strcmp(extension.extensionName, extensionName) == 0;
    });

    if (!found) {
        throw std::runtime_error(std::string("Required OpenXR extension unavailable: ") + extensionName);
    }
}

void XrContext::shutdown() noexcept {
    systemId_ = XR_NULL_SYSTEM_ID;
    passthroughEnabled_ = false;
    if (instance_ != XR_NULL_HANDLE) {
        xrDestroyInstance(instance_);
        instance_ = XR_NULL_HANDLE;
    }
}

} // namespace voxen::xr
