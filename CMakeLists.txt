cmake_minimum_required(VERSION 3.22)
project(voxen LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(ANDROID)
    find_package(OpenXR REQUIRED CONFIG)
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
