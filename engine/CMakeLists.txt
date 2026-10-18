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
    voxen_embed_shader(voxenPlasmaFrag voxen_plasma.frag)
    voxen_embed_shader(voxenSpheresFrag voxen_spheres_intersections.frag)
    voxen_embed_shader(voxenVolumeVert voxen_volume.vert)
    voxen_embed_shader(voxenMandelbulbFrag voxen_mandelbulb.frag)
    voxen_embed_shader(voxenAttractorVert voxen_attractor.vert)
    voxen_embed_shader(voxenAttractorFrag voxen_attractor.frag)
    voxen_embed_shader(fractalVert fractal.vert)
    voxen_embed_shader(laserFrag laser.frag)
    voxen_embed_shader(liquidRoomFrag liquid_room.frag)
    voxen_embed_shader(solidWallFrag solid_wall.frag)
    voxen_embed_shader(meshWireVert mesh_wire.vert)
    voxen_embed_shader(meshWireFrag mesh_wire.frag)
    voxen_embed_shader(meshDepthVert mesh_depth.vert)
    voxen_embed_shader(meshDepthFrag mesh_depth.frag)
    voxen_embed_shader(matrixRainFrag matrix_rain.frag)
    voxen_embed_shader(heppGlowFrag hepp_glow.frag)
    voxen_embed_shader(phiFieldFrag phi_field.frag)
    voxen_embed_shader(mandelbrotZoomFrag mandelbrot_zoom.frag)
    voxen_embed_shader(carouselFrag carousel.frag)
    voxen_embed_shader(imguiPanelFrag imgui_panel.frag)
endif()


# Native Vulkan UI implementation. Dear ImGui is compiled as a library so that
# the XR spatial-panel renderer can consume ImDrawData independently of input.
# Pin a tag for reproducibility; no SDL/GLFW backend in the Quest application.
option(VOXEN_WITH_IMGUI "Build native Dear ImGui core and Vulkan backend" ON)
if(VOXEN_WITH_IMGUI)
    include(FetchContent)
    FetchContent_Declare(voxen_dear_imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG v1.91.9b
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(voxen_dear_imgui)
    add_library(voxen_imgui STATIC
        ${voxen_dear_imgui_SOURCE_DIR}/imgui.cpp
        ${voxen_dear_imgui_SOURCE_DIR}/imgui_draw.cpp
        ${voxen_dear_imgui_SOURCE_DIR}/imgui_tables.cpp
        ${voxen_dear_imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${voxen_dear_imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp
        ui/src/CarouselUi.cpp
        ui/src/ImGuiSurface.cpp
    )
    target_include_directories(voxen_imgui PUBLIC
        ${voxen_dear_imgui_SOURCE_DIR}
        ${voxen_dear_imgui_SOURCE_DIR}/backends
        ${CMAKE_CURRENT_SOURCE_DIR}/ui/include)
    if(ANDROID)
        target_link_libraries(voxen_imgui PUBLIC vulkan)
    elseif(WIN32)
        target_link_libraries(voxen_imgui PUBLIC Vulkan::Vulkan)
    endif()
endif()

add_library(voxen_engine STATIC
    core/src/Engine.cpp
    audio/src/AmbientMusic.cpp
    audio/src/Mp3Library.cpp
    xr/src/XrContext.cpp
    xr/src/XrSession.cpp
    xr/src/MetaSceneProvider.cpp
    render/src/VulkanContext.cpp
    render/src/XrSwapchainRenderer.cpp
    ${VOXEN_SHADER_HEADERS}
)

target_include_directories(voxen_engine
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/core/include
        ${CMAKE_CURRENT_SOURCE_DIR}/audio/include
        ${CMAKE_CURRENT_SOURCE_DIR}/xr/include
        ${CMAKE_CURRENT_SOURCE_DIR}/render/include
        ${CMAKE_CURRENT_SOURCE_DIR}/scene/include
        ${VOXEN_GENERATED_DIR}
)

target_compile_definitions(voxen_engine PUBLIC XR_USE_GRAPHICS_API_VULKAN)
if(ANDROID)
    target_link_libraries(voxen_engine PUBLIC aaudio)
    target_compile_definitions(voxen_engine PUBLIC XR_USE_PLATFORM_ANDROID)
elseif(WIN32)
    target_compile_definitions(voxen_engine PUBLIC XR_USE_PLATFORM_WIN32)
endif()

target_link_libraries(voxen_engine PUBLIC OpenXR::openxr_loader)
if(WIN32)
    target_link_libraries(voxen_engine PUBLIC Vulkan::Vulkan)
endif()

target_compile_features(voxen_engine PUBLIC cxx_std_20)

if(VOXEN_WITH_IMGUI)
    target_link_libraries(voxen_engine PUBLIC voxen_imgui)
    target_compile_definitions(voxen_engine PUBLIC VOXEN_WITH_IMGUI=1)
endif()
