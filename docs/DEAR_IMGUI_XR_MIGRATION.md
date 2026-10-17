# Dear ImGui XR migration — first integrated offscreen prototype

Branch: `feature/mr-layered-environments`

## Implemented in this iteration
- Native ImGui context and widgets, pinned v1.91.9b Vulkan backend.
- `ImGuiSurface`: 512×672 RGBA8 Vulkan image, render pass/framebuffer, Vulkan descriptor pools, sampler and descriptor set.
- CPU draw data submitted to an offscreen Vulkan pass once per stereo frame.
- Spatial textured quad composited into each XR view using a dedicated descriptor-set pipeline.
- Right-controller ray converted to UI panel UVs; trigger forwarded to ImGui mouse events.
- Visible controller ray clipped to UI panel intersection.
- Legacy green GLSL menu pipeline retained in sources as recovery option; no longer drawn when ImGui is enabled.

## Validation still required
**No local Android build or headset test was possible in this environment.** The change is not confirmed working.
- Build with `cd android && ./gradlew assembleDebug`.
- Launch, inspect `adb logcat -s Voxen:E Voxen:I AndroidRuntime:E`.
- Confirm readable ImGui panel near left controller with correct stereo convergence and input.
- Confirm `Liquid Room` checkbox/opacity UI exists; it is NOT yet wired to actual Liquid Room renderer settings.
- Ensure no Vulkan validation errors, flicker, missing font atlas, or alpha blend issues.
- Continue porting legacy StereoKit page controls.
- Remove the legacy GLSL pipeline after device QA and rollback readiness.

## Architecture
The image is rendered once per XR frame, then sampled by both eyes. GPU completion is serialized by the existing renderer fence. The offscreen attachment starts at UNDEFINED every frame and finishes SHADER_READ_ONLY_OPTIMAL; explicit subpass dependency makes color writes visible to fragment sampling.

Future optimization: per-frame resources and asynchronous submissions to avoid GPU waits. Current path favors correctness and debugging.
