# ADR 0001 — OpenXR + Vulkan as Voxen's primary rendering architecture

Status: Accepted for phased migration (2026-10-17).

## Decision
Use native C++20/OpenXR for session, input, passthrough and Meta-specific scene/anchor extensions. Use Vulkan for the primary graphics renderer, GPU timestamps, compute effects, voxel instancing and environment materials. Keep existing StereoKit path functional until feature parity; do not delete or rewrite the prototype in one step.

## Device providers
- Quest 3: integrate Meta native OpenXR scene understanding / scene mesh and spatial anchors directly. The MRUK concepts are useful, but the Unity/Unreal toolkit itself is not a requirement for the native C++ target.
- Android phone in a separate future target: consider ARCore as an alternate scene provider behind a provider interface; do not assume ARCore's Play Services runtime operates on Quest 3.
- SceneProvider interface: surface ids, transform, classification, bounds, optional mesh, optional depth, anchors, confidence, tracking status. All inputs capability-checked and gracefully degraded.

## Rendering
- Logical order: immersive environment, visible physical world, spatial voxels, UI + head-locked diagnostics.
- OpenXR passthrough alpha/composition must be validated in headset; avoid assuming the passthrough camera texture is exposed for a custom Sobel compute pass.
- Spatial occlusion and safety are separate from artistic rendering; always retain device boundary features and quick OFF.
- Vulkan GPU timing: calibrated timestamp period and query pool around rendering/compute passes, resolved asynchronously; show N/A if unsupported.
- HUD: app callback CPU time is not CPU frame time; it is explicitly labeled, 72Hz frame budget 13.89ms only as reference. GPU remains N/A until instrumented. Other refresh rates require runtime-adaptive threshold.

## Migration order
1. Verify Quest headset rendering, compositor and passthrough behavior; profile baseline.
2. Add Vulkan CPU and GPU timing, head-locked HUD and runtime refresh budget.
3. Native surface registry through Meta OpenXR extensions; test no-permission/no-scene states.
4. Port one liquid wall shader and one voxel artwork to Vulkan, with real-world occlusion if supported.
5. Share temporal + audio modulation across systems.
6. Port remaining StereoKit menus/tools and retire its runtime only after regression QA.
