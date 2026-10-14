# Voxen native OpenXR/Vulkan experiment

This branch deliberately bypasses StereoKit. It builds the existing native backend in
`engine/` + `platform/`, using Vulkan rendering and direct OpenXR lifecycle.

## Current state
- CMake no longer fetches, compiles or links StereoKit.
- Direct OpenXR extension discovery enables Meta Scene API dependencies together,
  when all four are advertised.
- Android declares the anchor permission needed by Meta Scene.
- The existing native backend still provides stereo frames, passthrough, controller
  inputs and its original Vulkan renderer.
- **There is currently no wall query, no WALL_FACE classification and no wall shader.**
- **The full StereoKit voxel painter, shader library, carousel, persistence and
  artwork interactions have not yet been migrated.**

## Next milestones
1. Extract the existing artwork/voxel algorithms into engine-level C++ independent
   of StereoKit types. Keep saved scene compatibility.
2. Create Vulkan pipelines for the procedural material library (GLSL/SPIR-V or
   HLSL compiled to SPIR-V) and instanced voxel meshes.
3. Implement native XR Scene Manager: async query events, spatial entity labels,
   bounds, XrSpace localization and wall quads.
4. Raycast controller rays to quads, convert hits to wall-local UV coordinates,
   render a diagnostic overlay, then bind shader materials and paint masks.
5. Finish UI interactions with native Vulkan panel renderer and avoid
   introducing another UI framework.

## Try it
```bash
git fetch origin
git switch experiment/native-openxr
# Build with your existing Android Studio/Gradle workflow.
# VOXEN_USE_STEREOKIT is no longer an option in this branch.
```

This is an experimental migration branch, not a replacement for the working main.

## Native carousel prototype
- The seven pages match the StereoKit layout: COLORS, SHAPES, ALGORITHMS,
  SHADERS, PARAMS, ANIMATIONS, SAVE / LOAD.
- Panel follows the **left controller**, with its bottom approximately at hand
  height and an upright facing orientation.
- Left thumbstick horizontal: switch pages.
- Right thumbstick horizontal: switch the highlighted choice.
- Right trigger: select an option for COLORS, SHAPES, ALGORITHMS, SHADERS;
  toggles AIM on PARAMS.
- Titles and options use an embedded 5x7 bitmap font rendered as Vulkan cubes.
- **Current limitation:** these are UI model selections; they are not wired into
  the old StereoKit painting/algorithm engine. PARAMS, ANIMATIONS and SAVE/LOAD
  have no working controls yet. Trigger conflicts with artwork grab interactions
  still need ray-to-panel hit testing and input capture.
- This implementation favors portability and testability over performance:
  glyphs currently make many cube draw calls; replace with instancing / font
  atlas once visual positioning is verified in headset.
- No Quest or Windows build/visual validation has been performed here.
