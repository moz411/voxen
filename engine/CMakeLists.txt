add_library(voxen_engine STATIC
    core/src/Engine.cpp
    xr/src/XrContext.cpp
    render/src/VulkanContext.cpp
)

target_include_directories(voxen_engine
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/core/include
        ${CMAKE_CURRENT_SOURCE_DIR}/xr/include
        ${CMAKE_CURRENT_SOURCE_DIR}/render/include
)

target_compile_definitions(voxen_engine
    PUBLIC
        XR_USE_PLATFORM_ANDROID
        XR_USE_GRAPHICS_API_VULKAN
)

target_link_libraries(voxen_engine PUBLIC OpenXR::openxr_loader)

target_compile_features(voxen_engine PUBLIC cxx_std_20)
