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

## Integration progress (2026-10-23)

- Runtime snapshots of up to 256 live Voxens are versioned and stored under Android private data. Scene controls live in the four-page XR carousel. A saved snapshot is not yet a full asset graph; audio playback is intentionally not resumed on load.
- `FloorPortalPlacement.hpp` computes controller-ray intersections only with tracked Meta Scene floor rectangles; no synthetic fallback floor is used.
- `PortalManager.hpp` converts verified LOCAL floor hits into persistent-anchor-local poses, and hides saved placements when the anchor is unlocalized. A different anchor identity is rejected. Host smoke tests cover origin changes, loss of localization, and deletion.
- Renderer has `setLocalizedGalleryRoom(...)`, `visibleGalleryPortals()` and `placeGalleryPortalFromRightController()`. The SCENES page exposes a guarded placement button.
- **Not yet connected:** the Meta Scene/OpenXR runtime must supply a genuinely localized persistent room anchor to `setLocalizedGalleryRoom`. Without it the button correctly remains disabled. Do not synthesize a room ID or anchor pose from LOCAL tracking.
- **Not yet rendered:** iridescent floor rings, low-cost scene previews, portal picking, GPU resource unload/reload, and MP3-driven shader features. Current code is not proof of headset functionality.
- Host smoke tests are defined under `tests/CMakeLists.txt`, independently of Android/Vulkan. Run with `cmake -S tests -B /tmp/voxen-gallery-tests && cmake --build /tmp/voxen-gallery-tests && ctest --test-dir /tmp/voxen-gallery-tests --output-on-failure`.

## Immersive-mode prototype

The SCENES carousel now exposes ENTER IMMERSIVE SCENE and EXIT TO GALLERY.
Entry validates and restores a serialized runtime snapshot. Exit stops ambient
and MP3 playback and releases the CPU Voxen instance vector capacity.
**This is not the complete lifecycle described above:** shared Vulkan pipelines,
mesh caches and GPU allocations remain resident; no claim of VRAM reclamation is
made. The gallery renderer still needs visible rings, scene picking, and real
Meta persistent-anchor localization. The UI enter action is intentionally
available for saved scenes even before room localization, to test snapshot
roundtrips independently of the Meta Scene service.

## Vulkan ring pass

`gallery_portal.frag` now draws animated iridescent floor rings in the
stereo Vulkan pass using the existing `fractal.vert` quad, one shared
pipeline and no per-ring GPU allocations. Rendering is bounded to 64 rings
per eye. It consumes `PortalManager::visible()` and therefore intentionally
draws nothing until a real persistent Meta room anchor has been localized.
Rings are hidden while an immersive scene is active. The cyan Sobel
passthrough configuration is unchanged.

The gallery index and portal validation now use `loadSceneMetadata()`
rather than reading each saved binary payload. Remaining work includes
Meta anchor localization, portal picking and previews, correct gallery
resource ownership, and Quest device verification. No headset render
or CI success has been verified for this branch.
