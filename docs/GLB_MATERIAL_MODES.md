# GLB material modes (experimental)

Branch: `feature/glb-original-materials-v1`

The GLB carousel now offers **ORIGINAL** and **VOXEN**.
- ORIGINAL renders each primitive's glTF `pbrMetallicRoughness.baseColorFactor` with a dedicated simple directional-light GLSL shader. It uses the existing vertex normals and preserves the glTF node hierarchy.
- VOXEN keeps the previous animated artistic Mandelbulb-style color shader.
- Selection changes the Vulkan pipeline at draw time, without re-importing the model.
- This change does NOT yet display embedded base-color textures, metallic-roughness textures, normal maps, emissive textures, alpha blending/masking, or faithfully reproduce physically-based rendering. Models that encode all colors in textures may therefore appear white/gray in ORIGINAL.
- The current GLB preview still holds only one model at a time; scene/gallery persistence is unchanged.

## Local build and Quest verification

```sh
cd ~/voxen
git fetch origin
git switch feature/glb-original-materials-v1
cd android
./gradlew assembleDebug && adb install -r app/build/outputs/apk/debug/app-debug.apk
```

Place `sun.glb` and `portal.glb` under
`/sdcard/Android/data/org.voxen.xr/files/assets`. Open GLB, select each
model and toggle ORIGINAL/VOXEN. Confirm stereo appearance, no GPU validation
errors, and unchanged passthrough cyan Sobel.

No Android/Quest build or hardware validation has been performed by the assistant.

## Embedded base-color textures — experimental implementation

- `cgltf` extracts embedded PNG/JPEG `image.bufferView` content and associates `material.pbrMetallicRoughness.baseColorTexture` with the primitive (TEXCOORD_0 only).
- `stb_image` decodes bounded RGBA8 data (up to 4096×4096 per image, 64 images); Vulkan uploads them as `VK_FORMAT_R8G8B8A8_SRGB`, transitions images to SHADER_READ_ONLY_OPTIMAL, creates linear-filtered repeat samplers and descriptor sets.
- The dedicated GLB textured shader samples UV0 and multiplies the image by the material's base-color factor and simple directional lighting. VOXEN mode remains procedural. Untextured primitives retain their factor-color shader.
- GPU images and descriptor pool are cleaned up when replacing GLB assets / renderer shutdown.

**NOT supported:** external resources; image data URIs; KTX2/BasisU; UV1 or `KHR_texture_transform`; metallic/roughness, normals, emissive textures; texture-specific glTF sampler settings; alpha blend/mask and double-sided materials; mipmaps; full metallic-roughness BRDF; multiple independently saved GLB instances.

**Validation status:** code committed, but Android/Quest 3 build, embedded image rendering and GPU validation have not been tested. Test with `sun.glb` and `portal.glb` using the instructions above. Do not merge this experimental branch until hardware validation.
