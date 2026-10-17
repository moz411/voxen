# Meta Scene -> Liquid Room (first native integration)

Branch: `feature/meta-scene-liquid-room`; branched from merged `main`.

## Implemented
- Optional OpenXR extensions: `XR_FB_spatial_entity`, `XR_FB_spatial_entity_query`, `XR_FB_scene`.
- Quest manifest requests `com.oculus.permission.USE_SCENE`.
- `MetaSceneProvider`: nonblocking `xrQuerySpacesFB` with 2D-bounded spatial-entity filter, consumption of query result/complete events, semantic labels and bounding boxes, `xrLocateSpace` against LOCAL at predicted display time.
- Only semantic `WALL_FACE` (not `INVISIBLE_WALL_FACE`) is mapped to `EnvironmentSurface`. Bounding-box offset is rotated into LOCAL space and half extents come from the box.
- Renderer draws MddXzl Liquid Room on up to 16 tracked physical wall rectangles; if no wall is tracked, retains the earlier synthetic demo plane.
- Existing Meta passthrough/edge style and Dear ImGui carousel unchanged.
- `VoxenScene` log tag traces extension availability, query results, count transitions.

## Quest test
```bash
cd ~/voxen
git fetch origin
git switch feature/meta-scene-liquid-room
git pull --ff-only
cd android
./gradlew assembleDebug
adb install -r app/build/outputs/apk/debug/app-debug.apk
adb logcat -s VoxenScene:I Voxen:I AndroidRuntime:E
```

### Requirements and caveats
1. Perform Quest **Space Setup** in the actual room; permit **spatial data access** to Voxen if prompted.
2. This first implementation has **not yet been compiled or device-tested** here. If it fails, capture first compiler error and logs.
3. Meta runtime extensions may be absent, unavailable, or denied. If no walls are delivered, the synthetic pane remains. Current app attempts one query on XR READY; a runtime permission granted only afterward can require app restart.
4. This version queries stored bounded 2D anchors. It does not yet invoke a fresh Scene Capture flow, rescan on demand, handle room-specific selection, or build geometry for door/window holes. Those are next.
5. Each visible wall is a rectangle, not an exact mesh. No environment depth occlusion yet. Guardian / safety boundaries remain necessary.
6. The original Shadertoy effect is GPU intensive. Wall count is bounded to 16 but surface fill still requires device GPU profiling before increasing coverage.
7. Do not merge to `main` before Quest validation.

## Next
- Verify `VoxenScene` logs and visible per-wall UV mapping.
- Add explicit rescan/retry and Space Capture action.
- Improve surface clipping and occlusion, add wall category toggles, enforce GPU budgets.
