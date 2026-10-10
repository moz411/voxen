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
