# Liquid Room — Shadertoy MddXzl

Source URL: https://www.shadertoy.com/view/MddXzl

## Status (2026-10-17)
User provided GLSL `Image` pass directly. Ported to `engine/render/shaders/liquid_room.frag` and embedded as SPIR-V by `engine/CMakeLists.txt`. A translucent experimental quad is currently drawn at a fixed transform by `XrSwapchainRenderer`.

**This quad is synthetic, not a Meta-detected wall.** The renderer's `EnvironmentSurfaceRegistry` still has no producer. Device compilation and Quest 3 validation have not been performed; the actual visual output is unknown.

## Changes from original
- `iTime` mapped to push constant time; `fragCoord / iResolution` mapped to quad-local UV.
- Raymarch max iterations limited to 8 (from 12); original acceleration step `dist * 10.0` removed in favor of bounded signed-distance stepping.
- Hit threshold raised to 0.003; miss pixels discarded.
- Original monochrome lighting tinted cyan, alpha capped at 0.42.
- Transparent blending enabled with depth writes disabled for this pass.
- On some scenes the optimized version can look different or show sparse/no hits; compare against source on Quest.

## Blockers and next tasks
- [ ] Confirm original author, explicit redistribution license and Shadertoy attribution before distributing the source or app.
- [ ] Build on Android and profile GPU; this is a high-cost fragment shader (27 neighbors per field sample).
- [ ] Visually verify quad facing, transparency, raymarch hit rate, passthrough and edges on device.
- [ ] Add an opt-in toggle; current prototype renders the quad unconditionally.
- [ ] Integrate native Meta OpenXR scene capture to populate tracked walls; project the material onto actual surfaces.
- [ ] Handle anchor recentering, plane normals and wall UV sizes; ensure fallback when scene data unavailable.
- [ ] Respect room safety: compositor outlines don't replace Guardian or physical collision awareness.
- [ ] Refine GPU timings to measure two-eye total and individual pass cost.
