cmake_minimum_required(VERSION 3.22)
project(voxen LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(ANDROID)
    find_package(OpenXR REQUIRED CONFIG)
elseif(WIN32)
    find_package(Vulkan REQUIRED)

    find_package(OpenXR QUIET CONFIG)
    if(NOT TARGET OpenXR::openxr_loader)
      find_path(OPENXR_INCLUDE_DIR openxr/openxr.h HINTS
        "$ENV{VULKAN_SDK}/Include"
        "$ENV{VULKAN_SDK}/include"
    )
    find_library(OPENXR_LOADER_LIBRARY NAMES openxr_loader HINTS
        "$ENV{VULKAN_SDK}/Lib"
        "$ENV{VULKAN_SDK}/lib"
    )
    if(NOT OPENXR_INCLUDE_DIR OR NOT OPENXR_LOADER_LIBRARY)
        message(FATAL_ERROR "OpenXR headers/loader not found. Install an OpenXR SDK (the Vulkan SDK commonly provides them).")
    endif()

    add_library(OpenXR::openxr_loader UNKNOWN IMPORTED)
    set_target_properties(OpenXR::openxr_loader PROPERTIES
        IMPORTED_LOCATION "${OPENXR_LOADER_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${OPENXR_INCLUDE_DIR}"
    )
    endif()
endif()

# Experimental StereoKit backend; old Vulkan backend remains the default.
option(VOXEN_USE_STEREOKIT "Build the StereoKit XR/UI prototype" OFF)
if(VOXEN_USE_STEREOKIT)
    include(FetchContent)
    set(SK_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(SK_BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)
    set(SK_PHYSICS OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(StereoKitC
        GIT_REPOSITORY https://github.com/StereoKit/StereoKit.git
        GIT_TAG v0.3.11
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(StereoKitC)
    # StereoKit's OpenXR headers are a private dependency; our edge extension
    # directly includes openxr/openxr.h and needs the header target too.
    if(TARGET OpenXR::headers)
        set(VOXEN_OPENXR_HEADERS_TARGET OpenXR::headers)
    elseif(TARGET OpenXR::openxr_loader)
        set(VOXEN_OPENXR_HEADERS_TARGET OpenXR::openxr_loader)
    else()
        message(FATAL_ERROR "StereoKit did not provide OpenXR headers")
    endif()

    if(ANDROID)
        add_library(voxen SHARED
            stereokit/src/main.cpp
            stereokit/android/android_main.cpp
            ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)
        target_include_directories(voxen PRIVATE
            ${ANDROID_NDK}/sources/android/native_app_glue)
        target_link_libraries(voxen PRIVATE StereoKitC ${VOXEN_OPENXR_HEADERS_TARGET} android log)
        skshaderc_compile_headers(voxen
            ${CMAKE_CURRENT_BINARY_DIR}/voxen_shaders/
            "-O3 -f -e -t e -i ${stereokitc_SOURCE_DIR}/tools/include -i ${CMAKE_CURRENT_SOURCE_DIR}/stereokit/shaders"
            stereokit/shaders/voxen_voxel.hlsl
            stereokit/shaders/voxen_sky_light.hlsl)
        target_include_directories(voxen PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/voxen_shaders/)
    else()
        add_executable(voxen_stereokit stereokit/src/main.cpp)
        target_link_libraries(voxen_stereokit PRIVATE StereoKitC ${VOXEN_OPENXR_HEADERS_TARGET})
        skshaderc_compile_headers(voxen_stereokit
            ${CMAKE_CURRENT_BINARY_DIR}/voxen_shaders/
            "-O3 -f -e -t e -i ${stereokitc_SOURCE_DIR}/tools/include -i ${CMAKE_CURRENT_SOURCE_DIR}/stereokit/shaders"
            stereokit/shaders/voxen_voxel.hlsl)
        target_include_directories(voxen_stereokit PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/voxen_shaders/)
    endif()
    # Generated HLSL header is included directly by main.cpp; depend on it
    # explicitly so Ninja never compiles main.cpp ahead of skshaderc.
    set_source_files_properties(stereokit/src/main.cpp PROPERTIES
        OBJECT_DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/voxen_shaders/voxen_voxel.hlsl.h")
    return()
endif()

add_subdirectory(engine)

if(ANDROID)
    add_library(voxen SHARED
        platform/android/src/main.cpp
        ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c
    )

    target_include_directories(voxen PRIVATE
        ${ANDROID_NDK}/sources/android/native_app_glue
    )

    target_link_libraries(voxen PRIVATE
        voxen_engine
        OpenXR::openxr_loader
        android
        log
        vulkan
    )
endif()


if(WIN32)
    add_executable(voxen_windows platform/windows/src/main.cpp)
    target_link_libraries(voxen_windows PRIVATE
        voxen_engine
        OpenXR::openxr_loader
        Vulkan::Vulkan
    )
endif()
