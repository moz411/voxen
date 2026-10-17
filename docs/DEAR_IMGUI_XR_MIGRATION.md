# Dear ImGui XR migration — work in progress

Goal: replace the GLSL-drawn carousel with a real interactive 3D Dear ImGui panel on Quest 3.

## Landed
- Dear ImGui core and Vulkan backend pinned in CMake (v1.91.9b), Android and Windows.
- UI model with eight page labels, Liquid Room toggle/opacity, animation speed and CPU/GPU fields.
- StereoKit is unaffected; existing GLSL carousel is kept as temporary fallback.

## Not yet wired (important)
- ImGui context lifecycle, font atlas upload, descriptor pool and Vulkan backend initialization.
- Offscreen RGBA panel image/framebuffer and correct Vulkan layout barriers.
- Rendering ImDrawData into panel texture, quad compositing with correct alpha.
- OpenXR controller-ray intersection with tracked panel plane; button/slider events to ImGuiIO.
- Actual connection of settings to renderer (Liquid Room currently drawn unconditionally).
- App validation on Quest 3, shader and pipeline perf tests.

Do not report this as a working ImGui replacement. At present the Vulkan build still displays the temporary shader-based green frame. The new widgets are compiled but not rendered.

Implementation notes:
- Use a per-frame offscreen VkImage (or properly synchronized reused image); sample as texture in the stereo projection pass.
- Correct physical panel size and stereo parallax must come from one tracked world transform, not two independent head-facing canvases.
- Keep real-world passthrough and edge enhancements as separate OpenXR compositor layers.
- Avoid feeding both the object-grab controller and ImGui click for the same trigger.
