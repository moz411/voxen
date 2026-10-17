# Liquid Room candidate: Shadertoy MddXzl

Source: https://www.shadertoy.com/view/MddXzl
Status: **BLOCKED — original GLSL and licensing information not yet acquired**.

Do not substitute an invented shader for this source. Need all Shadertoy passes:
- Common (if present)
- Image
- Buffer A/B/C/D (if used)
- iChannel0-3 type and source (textures, audio, keyboard, buffer outputs, cubemap)
- Author attribution and explicit reuse license or permission.

## Porting target

Voxen native Vulkan / OpenXR, branch feature/mr-layered-environments.

1. Preserve source code and attribution under `assets/shadertoy/MddXzl/` only when license allows redistribution.
2. Reconcile Shadertoy uniforms (`iResolution`, `iTime`, `iMouse`, `iChannel*`) with Vulkan descriptor sets / push constants.
3. Render in an environment-surface local-UV shader pass, with a native Vulkan pipeline and depth semantics validated against passthrough.
4. Use only true tracked walls from `EnvironmentSurfaceRegistry`; no fake wall should be described as scene-detected.
5. Provide fallback when Meta Scene permission is missing; opt-in standalone demonstration plane may be explicitly labeled as synthetic.
6. Preserve OpenXR passthrough and Meta compositor edge highlighting; it is not a camera Sobel pass.
7. Add disable control, brightness/opacity clamps and diagnostic GPU timestamps.
8. Device build, verify screen orientation, compositor alpha, tracking, clipping, CPU/GPU frametime and thermal stability on Quest 3.

## Shader source handoff
Open Shadertoy > Image > copy code; repeat for each referenced buffer and Common. Share pasted text or GLSL/ZIP files. Capture the iChannel configurations and the original author/license.
