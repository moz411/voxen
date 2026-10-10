set(VOXEN_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY "${VOXEN_GENERATED_DIR}")

if(ANDROID)
    set(VOXEN_GLSLC "${ANDROID_NDK}/shader-tools/windows-x86_64/glslc.exe")
    if(NOT EXISTS "${VOXEN_GLSLC}")
        set(VOXEN_GLSLC "${ANDROID_NDK}/shader-tools/linux-x86_64/glslc")
    endif()
else()
    find_program(VOXEN_GLSLC NAMES glslc HINTS
        "$ENV{VULKAN_SDK}/Bin"
        "$ENV{VULKAN_SDK}/bin"
    )
    if(NOT VOXEN_GLSLC)
        message(FATAL_ERROR "glslc not found. Install the Vulkan SDK or put glslc on PATH.")
    endif()
endif()

if(ANDROID OR WIN32)

    function(voxen_embed_shader NAME SOURCE)
        set(SPV "${VOXEN_GENERATED_DIR}/${NAME}.spv")
        set(HEADER "${VOXEN_GENERATED_DIR}/${NAME}.spv.hpp")
        add_custom_command(
            OUTPUT "${HEADER}"
            COMMAND "${VOXEN_GLSLC}" "${CMAKE_CURRENT_SOURCE_DIR}/render/shaders/${SOURCE}" -o "${SPV}"
            COMMAND "${CMAKE_COMMAND}" -DINPUT=${SPV} -DOUTPUT=${HEADER} -DSYMBOL=${NAME}
                    -P "${CMAKE_SOURCE_DIR}/cmake/EmbedBinary.cmake"
            DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/render/shaders/${SOURCE}" "${CMAKE_SOURCE_DIR}/cmake/EmbedBinary.cmake"
            VERBATIM
        )
        set(VOXEN_SHADER_HEADERS ${VOXEN_SHADER_HEADERS} "${HEADER}" PARENT_SCOPE)
    endfunction()

    voxen_embed_shader(cubeVert cube.vert)
    voxen_embed_shader(cubeFrag cube.frag)
    voxen_embed_shader(proceduralFrag procedural.frag)
    voxen_embed_shader(organicaVert organica.vert)
    voxen_embed_shader(fractalFrag fractal.frag)
    voxen_embed_shader(fractalVert fractal.vert)
endif()

add_library(voxen_engine STATIC
    core/src/Engine.cpp
    xr/src/XrContext.cpp
    xr/src/XrSession.cpp
    render/src/VulkanContext.cpp
    render/src/XrSwapchainRenderer.cpp
    ${VOXEN_SHADER_HEADERS}
)

target_include_directories(voxen_engine
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/core/include
        ${CMAKE_CURRENT_SOURCE_DIR}/xr/include
        ${CMAKE_CURRENT_SOURCE_DIR}/render/include
        ${VOXEN_GENERATED_DIR}
)

target_compile_definitions(voxen_engine PUBLIC XR_USE_GRAPHICS_API_VULKAN)
if(ANDROID)
    target_compile_definitions(voxen_engine PUBLIC XR_USE_PLATFORM_ANDROID)
elseif(WIN32)
    target_compile_definitions(voxen_engine PUBLIC XR_USE_PLATFORM_WIN32)
endif()

target_link_libraries(voxen_engine PUBLIC OpenXR::openxr_loader)
if(WIN32)
    target_link_libraries(voxen_engine PUBLIC Vulkan::Vulkan)
endif()

target_compile_features(voxen_engine PUBLIC cxx_std_20)
