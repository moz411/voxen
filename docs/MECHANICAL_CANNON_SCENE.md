# Mechanical Cannon — scene prototype

Branch: `feature/scene-mechanical-cannon`

## Implemented
- Header-only, deterministic C++ mechanical-cannon scene definition: 13 mesh-part descriptors (base, turret, barrel, breech, hydraulic pistons, five cooling rings, muzzle, energy core).
- Local viewer-relative layout: cannon pivot 10 m forward, with 3.25 m barrel extending toward viewer.
- Stateless timeline: reveal (0–5 s), charge (5–10 s), repeated projectile shots (after 10 s).
- Shot states: recoil, flash, audio pulse, charging, projectile position, emissive energy and dissolve.
- Seeded dispersion and hard 1.5 m spectator standoff, bounded active projectile count.
- Host-only C++ smoke test `tests/mechanical_cannon_smoke.cpp`.

## Mesh generation progress
- `MechanicalCannonMesh.hpp` generates indexed triangle meshes with normals (box and cylindrical proxies) for all 13 components; geometry is still CPU-side and not uploaded to Vulkan.
- `mechanical_cannon_smoke` is registered in `tests/CMakeLists.txt`; test execution remains pending.

## First Vulkan preview (experimental)
- SCENES carousel has an opt-in **MECHANICAL CANNON (DEV)** checkbox.
- A stereo Vulkan draw pass uses existing cube.vert and a new dark-steel/emissive fragment shader for the 13 cannon components; organic sphere pipeline draws live shot projectiles.
- When preview is enabled the XR projection layer clears opaque black (alpha=1), intended to hide passthrough in the projection region; runtime composition must be verified on Quest.
- The preview deliberately uses cuboid stand-ins rather than the CPU cylinder meshes; indexed mesh buffer upload is still pending.
- Existing gallery and editor may still draw other content over the preview. It is not a production scene or fully isolated immersive lifecycle.
- Disable the checkbox to restore the usual alpha-clear MR rendering. Keep headset guardian/boundary protections active.

## Not implemented / integration required
- This is a **simulation/geometry descriptor**, not a working XR scene. Renderer must upload generated meshes and turn part descriptors into actual instanced draws, refine hollow barrel/rings, transform them into headset reference space, apply recoil and emissive shaders, and render projectile material IDs using the existing Voxen shader system.
- Disable passthrough only while immersed and retain a reliable exit to cyan-Sobel MR gallery. Physical guardian/boundary protection must remain active. Virtual darkness does not make the physical room safe.
- Integrate the timeline with spatial MP3 playback; `audioPulse` is a signal, not sound.
- Serialize scene config and time into the gallery's versioned scene payload, and add scene selection/preview; the current repository's gallery snapshot is not a full declarative scene loader.
- Validate Quest 3 frame time, depth, audio, shader behavior and tracking. **No headset test or build is claimed.**

## Local host smoke test

```sh
c++ -std=c++17 -Iengine/scene/include tests/mechanical_cannon_smoke.cpp -o /tmp/mechanical_cannon_smoke
/tmp/mechanical_cannon_smoke
```


## Gallery integration
- The cannon is now seeded as a persistent scene record named **Canon mecanique** in the app-private SceneRepository (stable ID `voxen-mechanical-cannon-v1`).
- Its serialized runtime is version 3, with `sceneKind=1`; version 1 and 2 snapshots remain readable.
- The SCENES list uses the normal SELECT / ENTER / DISPOSE flow; DISPOSE places a standard anchored iridescent portal, and entering the portal activates the cannon while exiting releases it.
- The old DEV checkbox is removed. The renderer draws the cannon only while its scene is immersive; a scene-relative clock restarts its timeline on each entry.
- Gallery preview is the existing lightweight proxy sphere (not a rendered cannon thumbnail).
- The first installation seeds the scene if missing; deleting it removes placements but it is reseeded on a subsequent app startup. This built-in-demo lifecycle still needs a permanent removal policy.
- The black opaque projection pass and cube-based mechanical parts still need headset verification.
