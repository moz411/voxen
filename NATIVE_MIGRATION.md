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
5. Port UI interactions using a small XR-native panel renderer instead of
   introducing another UI framework.

## Try it
```bash
git fetch origin
git switch experiment/native-openxr
# Build with your existing Android Studio/Gradle workflow.
# VOXEN_USE_STEREOKIT is no longer an option in this branch.
```

This is an experimental migration branch, not a replacement for the working main.
