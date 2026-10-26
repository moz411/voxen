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

    voxen_embed_shader(torusKnotVert torus_knot.vert)
    voxen_embed_shader(spikeBloomVert spike_bloom.vert)
    voxen_embed_shader(jellyfishVert jellyfish.vert)
    voxen_embed_shader(earthwormVert earthworm.vert)
    voxen_embed_shader(metaballsVert metaballs.vert)
    voxen_embed_shader(cloudVert cloud.vert)
    voxen_embed_shader(cloudFrag cloud.frag)
    voxen_embed_shader(organicSphereVert organic_sphere.vert)
    voxen_embed_shader(organicSphereFrag organic_sphere.frag)
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
    voxen_embed_shader(handOutlineFrag hand_outline.frag)
    voxen_embed_shader(liquidRoomFrag liquid_room.frag)
    voxen_embed_shader(solidWallFrag solid_wall.frag)
    voxen_embed_shader(mandelbulbLitVert mandelbulb_lit.vert)
    voxen_embed_shader(mandelbulbLitFrag mandelbulb_lit.frag)
    voxen_embed_shader(glbOriginalVert glb_original.vert)
    voxen_embed_shader(glbOriginalFrag glb_original.frag)
    voxen_embed_shader(glbTexturedVert glb_textured.vert)
    voxen_embed_shader(glbTexturedFrag glb_textured.frag)
    voxen_embed_shader(meshWireVert mesh_wire.vert)
    voxen_embed_shader(meshWireFrag mesh_wire.frag)
    voxen_embed_shader(meshDepthVert mesh_depth.vert)
    voxen_embed_shader(meshDepthFrag mesh_depth.frag)
    voxen_embed_shader(matrixRainFrag matrix_rain.frag)
    voxen_embed_shader(heppGlowFrag hepp_glow.frag)
    voxen_embed_shader(phiFieldFrag phi_field.frag)
    voxen_embed_shader(soundDemoFrag sound_demo.frag)
    voxen_embed_shader(midnightRideFrag midnight_ride.frag)
    voxen_embed_shader(seascapeFrag seascape.frag)
    voxen_embed_shader(octgramsFrag octgrams.frag)
    voxen_embed_shader(rainbowSpiralFrag rainbow_spiral.frag)
    voxen_embed_shader(mobiusSpiralFrag mobius_spiral.frag)
    voxen_embed_shader(thereminWaveFrag theremin_wave.frag)
    voxen_embed_shader(mandelbrotZoomFrag mandelbrot_zoom.frag)
    voxen_embed_shader(carouselFrag carousel.frag)
    voxen_embed_shader(galleryPortalFrag gallery_portal.frag)
    voxen_embed_shader(mechanicalCannonFrag mechanical_cannon.frag)
    voxen_embed_shader(imguiPanelFrag imgui_panel.frag)
    voxen_embed_shader(panoramaVert panorama.vert)
    voxen_embed_shader(panoramaFrag panorama.frag)
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

# Optional CPU GLB mesh decoder. cgltf is a small header-only C parser.
# Disable with -DVOXEN_WITH_GLB_IMPORT=OFF for offline/legacy builds.
option(VOXEN_WITH_GLB_IMPORT "Build CPU glTF/GLB mesh decoder" ON)
if(VOXEN_WITH_GLB_IMPORT)
    include(FetchContent)
    FetchContent_Declare(voxen_cgltf
        GIT_REPOSITORY https://github.com/jkuhlmann/cgltf.git
        GIT_TAG v1.15
        GIT_SHALLOW TRUE
    )
    FetchContent_GetProperties(voxen_cgltf)
    if(NOT voxen_cgltf_POPULATED)
        FetchContent_Populate(voxen_cgltf)
    endif()
    FetchContent_Declare(voxen_stb
        GIT_REPOSITORY https://github.com/nothings/stb.git
        GIT_TAG master
        GIT_SHALLOW TRUE
    )
    FetchContent_GetProperties(voxen_stb)
    if(NOT voxen_stb_POPULATED)
        FetchContent_Populate(voxen_stb)
    endif()
    add_library(voxen_cgltf STATIC scene/src/CgltfImplementation.cpp)
    target_include_directories(voxen_cgltf PUBLIC ${voxen_cgltf_SOURCE_DIR})
    target_include_directories(voxen_cgltf PUBLIC ${voxen_stb_SOURCE_DIR})
endif()

add_library(voxen_engine STATIC
    core/src/Engine.cpp
    $<$<BOOL:${VOXEN_WITH_GLB_IMPORT}>:${CMAKE_CURRENT_SOURCE_DIR}/scene/src/GlbMeshDecoder.cpp>
    $<$<BOOL:${VOXEN_WITH_GLB_IMPORT}>:${CMAKE_CURRENT_SOURCE_DIR}/render/src/GlbGpuPreview.cpp>
    audio/src/AmbientMusic.cpp
    audio/src/Mp3Library.cpp
    xr/src/XrContext.cpp
    xr/src/XrSession.cpp
    xr/src/MetaSceneProvider.cpp
    render/src/VulkanContext.cpp
    render/src/XrSwapchainRenderer.cpp
    $<$<BOOL:${VOXEN_WITH_GLB_IMPORT}>:${CMAKE_CURRENT_SOURCE_DIR}/render/src/PanoramaImage.cpp>
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

if(VOXEN_WITH_GLB_IMPORT)
    target_link_libraries(voxen_engine PUBLIC voxen_cgltf)
    target_compile_definitions(voxen_engine PUBLIC VOXEN_WITH_GLB_IMPORT=1)
endif()
