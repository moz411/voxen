# GLB imports — Voxen v1

Branch: `feature/glb-scene-import-v1`

## Added

- Header-only, portable C++20 GLB 2.0 container reader at
  `engine/scene/include/voxen/scene/GlbAsset.hpp`.
- Bounded file reads (64 MiB default), little-endian header/chunk validation,
  JSON + optional binary chunk extraction, rejection of duplicate BIN chunks.
- No change to Quest rendering, OpenXR/Meta anchors, Sobel, scene persistence,
  menus or current gallery transitions.

## Integration design (not implemented)

1. Parse glTF JSON, bufferViews/accessors, nodes, meshes, PBR materials and textures
   using a maintained parser, with explicit limits on vertices, pixels, nodes,
   scene size and decompressed GPU memory.
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
The current module only validates the container envelope; it does not yet render
mesh objects, load external resources, interpret glTF nodes, or import animations.
There is no Quest hardware or Android build validation for this branch.

## Independent host smoke test

```sh
c++ -std=c++20 -Iengine/scene/include tests/glb_container_smoke.cpp -o /tmp/voxen-glb-smoke
/tmp/voxen-glb-smoke
```
