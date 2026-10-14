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

## Native painting prototype (October 2026)
- Right controller trigger outside the carousel emits 25 mm cube voxels at a
  brush point 8.5 cm along the controller's aim ray.
- The right trigger is captured for UI or painting (not artwork grabbing).
- 25 mm stroke spacing, 4096 voxel hard cap, no persistent storage yet.
- The nine original material names map to a native GLSL/Vulkan fragment shader.
  Rasengan, Particles and Noise are approximate native ports, not pixel-identical
  copies of the StereoKit HLSL versions.
- Carousel selections COLOR and SHADER affect *newly painted* voxels.
- SHAPES and ALGORITHMS selections are visible in the UI but the native
  generator and non-cubic voxel geometry are not yet migrated.
- PARAMS, ANIMATIONS, SAVE / LOAD remain placeholders.
- Vulkan currently submits one draw call per voxel, requiring instancing later.
- **Not compiled or validated on Quest 3:** inspect shader/compiler output and
  device logs before merging to main.

## Native Scene API scanner checkpoint
- `engine/xr/include/voxen/xr/SceneScanner.hpp` now requests locally
  stored bounded 2D spatial entities after the OpenXR session starts.
- `XrSession::pollEvents()` forwards scene query events to the scanner;
  no competing event loop is introduced.
- Scanner records `XrSpace` and local 2D rectangular bounds per entity.
- Still missing: semantic WALL_FACE filtering, xrLocateSpace relative to
  the reference space, quad generation, persistent anchoring and paint masks.
- Still missing: GPU voxel instancing / draw call batching. Current native
  voxel drawing is one draw call per voxel and must not be considered optimized.
- Device build, runtime extension availability and scene capture permissions
  require validation on the Quest 3. This branch is an experiment.

## Wall diagnostic pass
- `SceneScanner` reads semantic labels and retains only entities with the
  exact comma-separated label `WALL_FACE` for wall rendering.
- `xrLocateSpace` resolves the wall pose relative to the app's LOCAL space
  at the OpenXR predicted display time (invalid poses are ignored).
- The native Vulkan renderer draws the bounded 2D wall rectangle as a 5 mm
  thin diagnostic box with animated shader preset NEON.
- Diagnostics are opaque at present: blending and wall-aligned masks remain
  pending. Scene component availability/enabling must be tested against the
  installed Quest OpenXR runtime before the result can be considered reliable.
- This is **not GPU instancing**. Both voxels and walls currently issue a draw
  command for each object. Instancing requires an instance buffer, matching
  vertex input and a separate batch pipeline.
- The implementation has not been built on Android nor verified in headset.
