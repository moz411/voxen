#include <voxen/render/VulkanContext.hpp>

#include <jni.h>
#include <vulkan/vulkan.h>
#include <openxr/openxr_platform.h>

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

template <typename T>
T getXrFunction(XrInstance instance, const char* name) {
    PFN_xrVoidFunction function = nullptr;
    checkXr(xrGetInstanceProcAddr(instance, name, &function), name);
    if (function == nullptr) {
        throw std::runtime_error(std::string("OpenXR function unavailable: ") + name);
    }
    return reinterpret_cast<T>(function);
}

} // namespace

VulkanContext::~VulkanContext() {
    shutdown();
}

void VulkanContext::initialize(XrInstance xrInstance, XrSystemId systemId) {
    if (instance_ != VK_NULL_HANDLE) {
        throw std::logic_error("Vulkan context is already initialized");
    }

    const auto getRequirements =
        getXrFunction<PFN_xrGetVulkanGraphicsRequirements2KHR>(xrInstance, "xrGetVulkanGraphicsRequirements2KHR");
    const auto createInstance =
        getXrFunction<PFN_xrCreateVulkanInstanceKHR>(xrInstance, "xrCreateVulkanInstanceKHR");
    const auto getGraphicsDevice =
        getXrFunction<PFN_xrGetVulkanGraphicsDevice2KHR>(xrInstance, "xrGetVulkanGraphicsDevice2KHR");
    const auto createDevice =
        getXrFunction<PFN_xrCreateVulkanDeviceKHR>(xrInstance, "xrCreateVulkanDeviceKHR");

    XrGraphicsRequirementsVulkan2KHR requirements{XR_TYPE_GRAPHICS_REQUIREMENTS_VULKAN2_KHR};
    checkXr(getRequirements(xrInstance, systemId, &requirements), "xrGetVulkanGraphicsRequirements2KHR");

    VkApplicationInfo appInfo{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    appInfo.pApplicationName = "Voxen";
    appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
    appInfo.pEngineName = "Voxen";
    appInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
    appInfo.apiVersion = VK_API_VERSION_1_1;

    VkInstanceCreateInfo vkInstanceInfo{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    vkInstanceInfo.pApplicationInfo = &appInfo;

    XrVulkanInstanceCreateInfoKHR xrInstanceInfo{XR_TYPE_VULKAN_INSTANCE_CREATE_INFO_KHR};
    xrInstanceInfo.systemId = systemId;
    xrInstanceInfo.pfnGetInstanceProcAddr = vkGetInstanceProcAddr;
    xrInstanceInfo.vulkanCreateInfo = &vkInstanceInfo;

    VkResult vkResult = VK_SUCCESS;
    checkXr(createInstance(xrInstance, &xrInstanceInfo, &instance_, &vkResult), "xrCreateVulkanInstanceKHR");
    if (vkResult != VK_SUCCESS) {
        shutdown();
        throw std::runtime_error("vkCreateInstance through OpenXR failed with VkResult " + std::to_string(vkResult));
    }

    XrVulkanGraphicsDeviceGetInfoKHR deviceGetInfo{XR_TYPE_VULKAN_GRAPHICS_DEVICE_GET_INFO_KHR};
    deviceGetInfo.systemId = systemId;
    deviceGetInfo.vulkanInstance = instance_;
    checkXr(getGraphicsDevice(xrInstance, &deviceGetInfo, &physicalDevice_), "xrGetVulkanGraphicsDevice2KHR");

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice_, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice_, &queueFamilyCount, queueFamilies.data());

    bool foundGraphicsQueue = false;
    for (uint32_t index = 0; index < queueFamilyCount; ++index) {
        if ((queueFamilies[index].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0) {
            graphicsQueueFamily_ = index;
            foundGraphicsQueue = true;
            break;
        }
    }
    if (!foundGraphicsQueue) {
        shutdown();
        throw std::runtime_error("No Vulkan graphics queue family available");
    }

    constexpr float queuePriority = 1.0F;
    VkDeviceQueueCreateInfo queueInfo{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
    queueInfo.queueFamilyIndex = graphicsQueueFamily_;
    queueInfo.queueCount = 1;
    queueInfo.pQueuePriorities = &queuePriority;

    VkDeviceCreateInfo vkDeviceInfo{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    vkDeviceInfo.queueCreateInfoCount = 1;
    vkDeviceInfo.pQueueCreateInfos = &queueInfo;

    XrVulkanDeviceCreateInfoKHR xrDeviceInfo{XR_TYPE_VULKAN_DEVICE_CREATE_INFO_KHR};
    xrDeviceInfo.systemId = systemId;
    xrDeviceInfo.pfnGetInstanceProcAddr = vkGetInstanceProcAddr;
    xrDeviceInfo.vulkanPhysicalDevice = physicalDevice_;
    xrDeviceInfo.vulkanCreateInfo = &vkDeviceInfo;

    checkXr(createDevice(xrInstance, &xrDeviceInfo, &device_, &vkResult), "xrCreateVulkanDeviceKHR");
    if (vkResult != VK_SUCCESS) {
        shutdown();
        throw std::runtime_error("vkCreateDevice through OpenXR failed with VkResult " + std::to_string(vkResult));
    }

    vkGetDeviceQueue(device_, graphicsQueueFamily_, 0, &graphicsQueue_);
}

void VulkanContext::shutdown() noexcept {
    if (device_ != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(device_);
        vkDestroyDevice(device_, nullptr);
        device_ = VK_NULL_HANDLE;
    }
    graphicsQueue_ = VK_NULL_HANDLE;
    physicalDevice_ = VK_NULL_HANDLE;

    if (instance_ != VK_NULL_HANDLE) {
        vkDestroyInstance(instance_, nullptr);
        instance_ = VK_NULL_HANDLE;
    }
}

} // namespace voxen::render
