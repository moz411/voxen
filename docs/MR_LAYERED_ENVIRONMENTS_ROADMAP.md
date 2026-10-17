# Roadmap — MR Layered Environments

Branch: `feature/mr-layered-environments`

## Current baseline (verified from README)
- C++20/OpenXR/Vulkan engine and experimental StereoKit backend coexist.
- Quest currently uses `XR_FB_passthrough` blue passthrough / white contours; **not** a Vulkan camera Sobel implementation.
- Panoramic skies are rendered inside the StereoKit projection layer; **not guaranteed** behind passthrough in compositor order.
- Scene planes via `XR_FB_scene` exist in an experimental StereoKit path, not validated on headset.
- Six lightweight sky presets and CPU voxel artwork animations exist; build, GPU profile and headset QA remain outstanding.

## P0 — Validate and establish layering
- [ ] Build Android APK and smoke-test on Quest 3; record startup, passthrough, sky presets, menus, voxel animations and crash logs.
- [ ] Profile CPU/GPU timings and thermal behavior on device with skies ON/OFF.
- [ ] Document exact active runtime path (StereoKit vs native Vulkan) and select primary renderer; avoid parallel implementations.
- [ ] Prove compositor order and alpha blending: environment < real-world perception < spatial voxels < UI. Account for native passthrough underlays and projection alpha; logical ordering alone is insufficient.
- [ ] Define a stable always-visible physical-world visibility policy and user override/emergency OFF; retain Meta Guardian/boundary features.
- [ ] Verify whether true camera Sobel is legally/technically available with selected Meta API and permissions; otherwise retain and label the existing Meta contour effect. Do not claim arbitrary camera-frame access.

## P1 — Scene-aware environment renderer
- [ ] Integrate room planes (walls/floor/ceiling) in the chosen rendering backend; handle permission denial, stale scene and no-scene fallback.
- [ ] Build `EnvironmentSurface` registry with stable IDs, classification, transform, dimensions, shader/material and enable flag.
- [ ] Map shader UV consistently across planes; handle geometry boundaries, coordinate spaces and recentering.
- [ ] Add presets: LIQUID WALLS, FRACTAL ROOM, NEON GEOMETRY, COSMIC FLOW, MATRIX PULSE, MINIMAL GLOW.
- [ ] Implement safe blending/transparency and luminance limits, retaining legible real-world contours.
- [ ] Introduce environmental shader parameters with bounded GPU workload.

## P2 — Spatial voxel entities
- [ ] Define `VoxelEntity` (UUID, artwork/algorithm, transform, bounds, material, creation time, TTL, state, motion controller).
- [ ] Support persistent anchored artworks and ephemeral particles/trails with deterministic expiration and bounded pools.
- [ ] Add motion behaviors: orbit, spline, procedural flow, Lorenz attractor, spawn/return, optional follow-user.
- [ ] Handle depth / real-world occlusion where supported; graceful fallback if depth unavailable.
- [ ] Keep voxels visually above environment while preserving scene readability.

## P3 — Shared timing and audio
- [ ] Create `ModulationBus` with monotonic timeline, BPM/phase, envelopes, normalized band energies and transient events.
- [ ] Audio input architecture: microphone or user-owned audio playback, permissions and user-visible privacy state.
- [ ] FFT (low/mid/high), onset and beat detection with smoothing and hysteresis.
- [ ] Bind both shader uniforms and voxel behaviors to modulation parameters; allow manual control without audio.
- [ ] Add presets for multi-layer musical scenes and parameter automation.

## P4 — Authoring, persistence and UX
- [ ] Build shader preset registry and tooling for GLSL/HLSL/Slang -> validated SPIR-V artifacts; hot reload only where supported, safe fallback on compile errors.
- [ ] Extend carousel: ENVIRONMENTS, VOXELS, MOTION, AUDIO, SCENE, SAVE/LOAD, including accessible OFF controls.
- [ ] Scene save format with versioning, references to material versions, anchors, motion and modulations.
- [ ] Automated tests for entity lifecycle, parameter clamping, scene serialization and timeline behavior.
- [ ] Headset regression matrix: passthrough clarity, hand/controller input, guardian, lost tracking, missing permissions, overheating, long sessions.

## First demonstrator: LIQUID ROOM
- Existing Quest passthrough and visible contours;
- One detected wall with an animated fluid shader;
- One persistent orbiting voxel artwork and a short-lived voxel trail;
- One manual BPM slider driving both the wall and voxel animation;
- OFF button instantly restoring unmodified passthrough;
- Measured frame times and thermal status on Quest 3.

## Architecture rule
These are logical rendering layers, not necessarily four literal compositing passes. Real-world visibility and collision awareness cannot rely on Sobel contours alone.
