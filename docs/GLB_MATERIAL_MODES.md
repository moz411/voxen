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
