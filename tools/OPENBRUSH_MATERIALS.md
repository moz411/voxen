# Open Brush material fidelity (work in progress)

The current `VXMSH001`/`VXMSH002` loader renders stroke geometry with a generic mesh shader.
It does **not** implement Open Brush materials, even when the geometry family is
`particle`. Native `.tilt` currently has a separate Dots-only Vulkan path.

## Reproduce the audit

```bash
node tools/audit_openbrush.mjs ../Sa_Tuna.tilt -o brush-audit.json
node tools/compile_openbrush.mjs ../Sa_Tuna.tilt -o Sa_Tuna.vxmesh --materials Sa_Tuna.materials.json\n# Optional compatibility output for older builds:\nnode tools/compile_openbrush.mjs ../Sa_Tuna.tilt -o Sa_Tuna-v1.vxmesh --format v1
```

To test the v2 and v1 files with the dependency-free host reader:

```bash
c++ -std=c++17 -Iengine/render/include tests/openbrush_compiled_scene_smoke.cpp -o /tmp/vxmesh-smoke
/tmp/vxmesh-smoke Sa_Tuna.vxmesh
/tmp/vxmesh-smoke Sa_Tuna-v1.vxmesh
```

## Compiled binary formats

Both formats start with an 8-byte ASCII magic, a little-endian u32 version and
a little-endian u32 mesh count. For each mesh: GUID[36], vertexCount u32,
indexCount u32, bounds[6] f32, positions[vertexCount*3] f32,
normals[vertexCount*3] f32, colors[vertexCount*4] f32,
UV0 floats, then indices[indexCount] u32.

- `VXMSH001` / v1: UV0[vertexCount*2] f32.
- `VXMSH002` / v2: UV0[vertexCount*4] f32 (UV0.xyzw).
- The v1 runtime decoder pads UV0.zw to zero. Both feed a 56-byte Vulkan
  vertex: position[3], normal[3], color[4], UV0[4].
- Missing UV components from the source geometry generator are zero-filled
  by the v2 compiler. No material or texture bytes are embedded yet.
- `VXMSH002` is **not** readable by an old runtime. Rebuild Voxen before
  copying a newly compiled file to the headset.

Both tools require `tools/vendor/three-tiltloader` (upstream source checkout)
and the dependencies used by `compile_openbrush.mjs`.

## Priority by stroke count (Sa_Tuna.tilt)

| Brush GUID | Strokes | Geometry | State |
| --- | ---: | --- | --- |
| 6a1cf9f9-032c-45ec-9b1d-a6680bee30f7 | 72 | particle | Native Dots exists; fidelity not verified |
| 70d79cca-b159-4f35-990c-f02193947fe8 | 39 | particle | Material identity unresolved |
| 79348357-432d-4746-8e29-0e25c112e3aa | 33 | hull | Material identity unresolved |
| 7136a729-1aab-bd24-f8b2-ca88b6adfb67 | 16 | ribbon | Material identity unresolved |

**Important:** 111 particle strokes is not the same as 111 correctly
rendered particle strokes. `VXMSH001` is geometry-only. Do not classify
a material as opaque/transparent/emissive from its geometry family alone.

## Next implementation gate

1. Resolve brush GUIDs against Open Brush's authoritative brush descriptors,
   including texture, blend mode, emission, and per-brush vertex semantics.
2. Extend v2 with explicit material/texture semantics; UV0.xyzw and backwards-compatible v1 decoding are now implemented.
3. Implement the two most frequent particle materials in Vulkan with
   per-eye depth/blend validation and a Quest 3 screenshot comparison.
4. Batch compatible instances and measure draw calls/GPU cost on device.

The optional `--materials` sidecar is a diagnostic report, **not** yet
consumed by the runtime. Its unresolved fields are intentional.

## Experimental Smoke Vulkan pass (Quest 3)

The compiled scene renderer now routes GUID
`70d79cca-b159-4f35-990c-f02193947fe8` to its own transparent
Vulkan pipeline, leaving other brushes on the generic geometry path.
Smoke vertex input uses `normal.xyz` as center and `UV0.z` as
per-particle rotation; it rebuilds camera-facing billboards for each eye.

Install the original Smoke texture from the Apache-2.0
`icosa-foundation/three-icosa` repository:

```bash
bash tools/install_openbrush_smoke.sh
cd android
./gradlew assembleDebug
adb install -r app/build/outputs/apk/debug/app-debug.apk
```

Recompile and push the **v2** scene if you have not done so since the
format update:

```bash
cd ..
node tools/compile_openbrush.mjs ../Sa_Tuna.tilt -o Sa_Tuna.vxmesh
adb push Sa_Tuna.vxmesh /sdcard/Android/data/org.voxen.xr/files/tilts/
```

Restart the app so the Smoke texture is loaded on scene entry. Logs:
`adb logcat -s VoxenOpenBrush:I`.

**Current visual limitations:** the Smoke animation is a cheaper
trigonometric curl-like flow, not the original three-icosa simplex curl
noise; transparent Smoke strokes are rendered after other meshes
without per-particle back-to-front sorting. Tint and timing parameters
are not yet brush-specific. If the PNG is absent, a synthetic texture
is generated for diagnosis and a warning is logged. This is an
experimental rendering milestone, **not exact Open Brush parity**.
