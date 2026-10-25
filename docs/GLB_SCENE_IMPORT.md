# GLB imports — Voxen v1

Branch: `feature/glb-scene-import-v1`

## Added

- Header-only, portable C++20 GLB 2.0 container reader at
  `engine/scene/include/voxen/scene/GlbAsset.hpp`.
- Bounded file reads (64 MiB default), little-endian header/chunk validation,
  JSON + optional binary chunk extraction, rejection of duplicate BIN chunks.
- No change to Quest rendering, OpenXR/Meta anchors, Sobel, scene persistence,
  menus or current gallery transitions.

## CPU mesh decoder (implemented, not yet device-tested)

- `GlbMeshDecoder.hpp/.cpp` uses pinned `cgltf v1.15` to decode a selected glTF scene.
- Extracts triangle vertices, indices, normals, UV0, metallic/roughness/base-color factors.
- Preserves node names, parent links and world transforms, with deduplicated mesh primitive references.
- Enforces encoded file, geometry and node limits and rejects external buffer URIs.
- Enable with `-DVOXEN_WITH_GLB_IMPORT=ON` (default); builds require dependency download on first configure.
- The feature currently produces **CPU arrays only**. It has no GPU upload, texture rendering, asset picker, scene persistence or headset runtime UI.

## Integration design (remaining)

1. Complete texture decoding and material extensions, enforcing decoded pixel and GPU budgets.
2. Convert meshes to a Vulkan vertex/index resource with per-asset lifetime.
   Preserve glTF hierarchy and coordinate-space conversions.
3. Add a scene asset record with content hash, import settings and transform.
   Objects and complete glTF scenes use the same hierarchical representation;
   multiple instances share GPU resources.
4. Expose a library picker in SCENES and object insertion in the immersive editor.
   Resolve path references only within the app-private assets directory.
5. On immersive exit release GLB GPU resources after completed Vulkan fences.
   Preserve Meta passthrough + Sobel and lightweight floor-portal gallery.

## Scope & caveats

GLB is a geometry/texture/animation container, not an Unreal or Unity level.
The current module decodes the GLB container, triangular meshes and scene hierarchy
in CPU memory. It does not yet render mesh objects, decode textures, load external
resources, or import animations. The CPU decoder has not been compiled on the Quest.
There is no Quest hardware or Android build validation for this branch.

## Independent host smoke test

```sh
c++ -std=c++20 -Iengine/scene/include tests/glb_container_smoke.cpp -o /tmp/voxen-glb-smoke
/tmp/voxen-glb-smoke
```

## Prototype Vulkan stereo (2026-10-25)

The branch now has an **experimental** Vulkan draw path for static GLB scenes:
- `GlbGpuPreview.cpp` allocates host-visible vertex/index buffers for glTF triangle primitives and frees them at shutdown or replacement.
- Each glTF scene node supplies its world transform; the prototype places the root 1.5 m in front of the initial OpenXR origin.
- The existing illuminated vertex/fragment shaders are reused. This gives a **procedural lit appearance**, not faithful glTF PBR materials or textures.
- A file at the app-private `voxen-gallery/assets/demo.glb` is detected on start and loaded only after Vulkan initialization.
- The model is drawn in editor or immersive mode, not while the room gallery is displayed. It is **not** yet attached to a saved scene.
- No GLB is bundled in the APK. Without the demo file there is no visual or memory change.

### Manual smoke test on a debug Quest 3 build

```sh
git fetch origin
git switch feature/glb-scene-import-v1
cd android
./gradlew assembleDebug
adb install -r app/build/outputs/apk/debug/app-debug.apk
adb push /path/to/triangles.glb /sdcard/Download/demo.glb
adb shell run-as org.voxen.xr mkdir -p files/voxen-gallery/assets
adb shell run-as org.voxen.xr sh -c 'cat /sdcard/Download/demo.glb > files/voxen-gallery/assets/demo.glb'
# Some run-as sandboxes cannot read shared storage; use adb exec-out/
# an alternate authorized app-private file transfer when that happens.
adb shell am force-stop org.voxen.xr
adb shell monkey -p org.voxen.xr 1
adb logcat -s Voxen:I AndroidRuntime:E
```

**Limitations / verification required:** Android NDK compile, Vulkan validation,
correct glTF scaling and material appearance, GPU timings, error reporting,
asset selection UI, external textures, PBR shader, 3D animations, scene persistence
and resource unload on immersive exit have **not been validated**. This is a
renderer prototype and should not be merged into main without Quest QA.
