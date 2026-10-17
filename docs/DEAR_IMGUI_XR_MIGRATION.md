# Dear ImGui XR migration — work in progress

Goal: replace the GLSL-drawn carousel with a real interactive 3D Dear ImGui panel on Quest 3.

## Landed
- Dear ImGui core and Vulkan backend pinned in CMake (v1.91.9b), Android and Windows.
- UI model with eight page labels, Liquid Room toggle/opacity, animation speed and CPU/GPU fields.
- Dear ImGui CPU context initialized in the native Vulkan renderer; NewFrame/drawCarousel/Render called once per XR stereo frame.
- Reusable OpenXR controller ray / oriented panel hit mapping in `SpatialPanelInput.hpp` (not yet fed into ImGuiIO).
- StereoKit is unaffected; existing GLSL carousel is kept as temporary fallback.

## Not yet wired (important)
- Font atlas upload, descriptor pool and Vulkan backend initialization (CPU ImGui lifecycle is wired).
- Offscreen RGBA panel image/framebuffer and correct Vulkan layout barriers.
- Rendering ImDrawData into panel texture, quad compositing with correct alpha.
- OpenXR controller-ray intersection with tracked panel plane; button/slider events to ImGuiIO.
- Actual connection of settings to renderer (Liquid Room currently drawn unconditionally).
- App validation on Quest 3, shader and pipeline perf tests.

Do not report this as a working ImGui replacement. At present the Vulkan build still displays the temporary shader-based green frame. The new widgets now generate ImDrawData every XR frame, but that draw data is not yet submitted to Vulkan and is **invisible** in the headset.

Implementation notes:
- Use a per-frame offscreen VkImage (or properly synchronized reused image); sample as texture in the stereo projection pass.
- Correct physical panel size and stereo parallax must come from one tracked world transform, not two independent head-facing canvases.
- Keep real-world passthrough and edge enhancements as separate OpenXR compositor layers.
- Avoid feeding both the object-grab controller and ImGui click for the same trigger.
