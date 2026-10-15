# Background source library (experimental)

The `BACKGROUNDS` panel now browses ten imported shader **source descriptors**.
The project archive used to build the descriptors is separate from this repository.
No third-party shader binary is shipped in this branch, and choosing a source
does **not** render it. The original legacy scene-plane GPU effect can still
be enabled independently for debugging.

## What remains before runtime backgrounds work

1. Import each original shader source and any referenced textures, with its
   exact original license and required attribution. Confirm commercial rights:
   CC BY-NC-SA cannot be treated as generally commercial-use compatible.
2. Port Shadered-specific entry points, uniforms (time, resolution, textures),
   and GLSL/HLSL/Rust-GPU semantics into the actual StereoKit shader format;
   compile and validate the generated shader payload for Quest.
3. Instantiate one material per supported background and bind its time,
   resolution, and intensity. Only enable selectable entries when material
   creation succeeds; add a real preview after successful rendering.
4. Implement the requested composition design. Putting an opaque XR passthrough
   layer above a projection background makes the latter invisible. Openings
   need a real mask, environment depth or a supported passthrough composition
   mechanism. Scene BOUNDED_2D planes are not per-pixel passthrough segmentation.
5. Ensure passthrough/Sobel visual treatment, opaque objects, UI, and voxel
   depth remain correct on Quest, including when Scene permission is denied.

## Manual checks

- Launch Quest without Scene access: `SOURCE: NONE`; UI and passthrough live.
- Browse ten sources: titles cycle, label says `SOURCE ONLY - NOT PORTED`.
- Toggle legacy scene-plane test effect independently.
- No source preset silently renders a different voxel shader.
- Verify OpenXR extensions, permission and actual Scene plane counts in logs.
- Confirm at least one real compiled background before claiming this feature complete.
