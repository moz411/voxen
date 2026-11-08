# Offline Open Brush compiler (experimental)

Branch: `feature/openbrush-offline-compiler`.

This is the first **geometry-only** compiler prototype, not yet a Quest runtime
integration. The existing gallery portal behavior is unchanged.

## Linux prerequisites

Node.js 22+ recommended. Install in a separate working directory to avoid
modifying Voxen's Android build dependencies:

```sh
mkdir -p /tmp/voxen-openbrush-tools
cd /tmp/voxen-openbrush-tools
npm init -y
npm install three three-tiltloader
```

The compiler resolves npm dependencies from its own location, so for this
prototype install the dependencies in the repository root instead:

```sh
cd ~/voxen
npm install --no-save --package-lock=false three three-tiltloader
node tools/compile_openbrush.mjs artwork.tilt -o artwork.vxmesh
```

Note: `TiltLoader.parse()` also requests brush shader assets; a local brush
asset path and asset loading strategy may be necessary before this runs
headlessly in Node. This is the next integration task. Do not deploy the
generated file to the headset yet: the Vulkan loader is not implemented.

## Binary format VXMSH001 v1

Little endian. Header: 8-byte magic `VXMSH001`, u32 version (1),
u32 mesh count. Each mesh: 36-byte brush/material identifier, u32 vertex
count, u32 index count, 6 x f32 AABB, then packed f32 positions (3),
normals (3), colors (4), UV0 (2) and u32 indices. Geometry is already in
Open Brush converted meter coordinates (Y up, Z reflected).

Next: extract geometry generation from material loading, add brush GUID
preservation, per-brush texture/material manifest, robust bounds and size
validation, C++ reader, and Vulkan pipelines.
