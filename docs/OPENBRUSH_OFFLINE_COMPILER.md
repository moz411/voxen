# Open Brush offline compiler — prototype VXMSH001

Branch: `feature/openbrush-offline-compiler`. The existing portal and Android renderer remain unchanged.

## Compile on Linux

From the Voxen repository root, with Node.js 20+:

```sh
npm install --no-save --package-lock=false three three-tiltloader fflate
node tools/compile_openbrush.mjs /path/to/artwork.tilt -o /tmp/artwork.vxmesh
```

The compiler reads the 16-byte .tilt header, decompresses the embedded ZIP,
parses strokes and uses Icosa's geometry generators directly, **without
instantiating TiltLoader or loading shaders**. It outputs JSON diagnostics
for stroke families, mesh count and triangle count.

## VXMSH001 v1

Little-endian: 8-byte magic `VXMSH001`, u32 version=1, u32 meshCount.
Each mesh contains: 36-byte brush GUID, u32 vertexCount, u32 indexCount,
six f32 AABB values, interleaved-by-attribute f32 positions (3 per vertex),
normals (3), colors (4), UV0.xy (2), and u32 indices.

Source positions and brush sizes are converted from decimeters to meters,
with reflected Z axis and quaternion X/Y components.

`engine/render/include/voxen/render/OpenBrushCompiledScene.hpp` contains
a bounded native reader. **The Vulkan GPU upload and gallery registration
are not yet connected**. Do not push .vxmesh to the headset expecting
it to appear in the gallery.

## Known limitations

- v1 intentionally discards tangents, extra packed UV components and UV1;
  particle/emissive and advanced materials therefore cannot yet be faithful.
- Each stroke is a separate mesh; batching by GUID is future work.
- Textures, brush shaders, blend modes and animation are not serialized.
- No end-to-end compilation or Quest runtime test has been performed yet.
- The compiler relies on upstream `three-tiltloader` package exports;
  dependency pinning and a fixture-based regression test are still needed.

## Next

Validate with a real .tilt fixture, add mesh batching and material manifest,
then upload the compiled meshes to Vulkan and register the compiled scenes
with the existing physical-entry portal system.
