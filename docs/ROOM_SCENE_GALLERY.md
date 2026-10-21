# Room scene gallery — implementation contract

Branch: `feature/room-scene-gallery`

## Current state
- Added a pure C++ data model in `engine/scene/include/voxen/scene/RoomGallery.hpp`.
- Added explicit Gallery / Entering / Immersive / Leaving transitions.
- **Not yet wired into Android/OpenXR/Vulkan, persistence or the UI.**
- No headset build/test has been performed for this branch.

## Required integration
1. **SceneRepository:** create UUID scene IDs; serialize all Voxen objects, shader parameters, animations, audio references and scene settings in a versioned document. Read/write under Android app-private storage. Atomic temp-file + rename, validation, migration, and crash recovery. Do not serialize pointers, Vulkan handles or OpenXR handles.
2. **Meta room:** enumerate localized rooms, persist room ID and anchor identity; re-localize on app launch. Resolve gallery placements in anchor-local coordinates; never reuse an unlocalized OpenXR LOCAL pose. If permission/room/anchor unavailable, offer retry or rescan rather than placing objects at fabricated positions.
3. **Floor portals:** use Meta Scene floor semantics and current tracking to project rings onto floor. No arbitrary fallback floor. Add animated iridescent shader with depth-aware rendering and controller hit tests.
4. **Previews:** generate small decimated/proxy geometry per saved scene; share preview shader/pipeline and keep MP3 and full scene simulations unloaded. Apply visibility/distance limits and profile Quest GPU.
5. **Lifecycle:** on enter, stop gallery preview updates, wait for outstanding GPU work/fences, release gallery-only Vulkan resources and CPU proxies, then load scene assets/audio/simulation. On exit stop audio/simulation, wait for GPU completion, destroy immersive-only resources, reload lightweight gallery previews. Keep XR session, passthrough, Sobel and shared renderer alive.
6. **Scene switching:** only enter after a completed cleanup; on load failure return to gallery with recoverable error. Preserve gallery state in compact CPU metadata.

## Acceptance tests
- Save three independent scenes, including material/animation/audio state.
- Place them in one localized Meta room, exit app, reboot headset and verify stable placements.
- Duplicate a scene placement without duplicating the scene asset.
- Enter/leave repeatedly and confirm no increasing allocated RAM/VRAM trend.
- Reject stale/missing anchor localization and avoid placing on an invented floor.
- Preserve cyan Sobel passthrough throughout transitions.
- Confirm preview GPU frame-time budget with multiple visible portals.

## Important
`engine/scene/include/voxen/scene/Scene.hpp` currently defines four demo artworks with a fixed-size array; it is **not** a complete saved-scene representation. Migration to full serialization is prerequisite to claiming multi-scene persistence.
