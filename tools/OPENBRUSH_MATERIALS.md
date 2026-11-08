# Open Brush material fidelity (work in progress)

The current `VXMSH001` loader renders stroke geometry with a generic mesh shader.
It does **not** implement Open Brush materials, even when the geometry family is
`particle`. Native `.tilt` currently has a separate Dots-only Vulkan path.

## Reproduce the audit

```bash
node tools/audit_openbrush.mjs ../Sa_Tuna.tilt -o brush-audit.json
node tools/compile_openbrush.mjs ../Sa_Tuna.tilt -o Sa_Tuna.vxmesh --materials Sa_Tuna.materials.json
```

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
2. Add an explicit versioned material payload to the compiled format; retain
   v1 decoding for existing scenes.
3. Implement the two most frequent particle materials in Vulkan with
   per-eye depth/blend validation and a Quest 3 screenshot comparison.
4. Batch compatible instances and measure draw calls/GPU cost on device.

The optional `--materials` sidecar is a diagnostic report, **not** yet
consumed by the runtime. Its unresolved fields are intentional.
