# Voxen StereoKit experimental backend

Opt-in native C++ prototype on a dedicated feature branch.

## Build (Quest 3)

From the repository root:

```bash
cd android
./gradlew assembleDebug -PvoxenStereoKit=true
adb install -r app/build/outputs/apk/debug/app-debug.apk
```

StereoKit is fetched from tag `v0.3.11` at CMake configure time. It requires a network connection, Git, compatible NDK and build dependencies. This path has not yet been compiled or validated on physical Quest 3.

Switch to the existing Vulkan backend by omitting `-PvoxenStereoKit=true`.
Both targets currently use package `org.voxen.xr`. If an installation has been signed using a different key, uninstall it first (this deletes app-local data).

## Scope

StereoKit owns OpenXR, rendering, UI and audio lifecycle in this backend; Voxen must not initialize a second XR session. Initial UI provides a spatial window, sliders and a reference cube. The hue slider is UI wiring only; painting, shader ports, controller brush poses, instancing and audio-reactive effects are not ported yet.

The legacy Vulkan engine and its files remain unchanged, and main is not affected.


## Native passthrough contours (Quest 3)

This branch adds an optional XR_FB_passthrough reconstruction layer with cyan
runtime-detected edges. The spatial UI shows `Contours cyan: OFF / ON` only
when the extension and layer initialized successfully. With edges OFF, the
existing transparent composition remains unchanged.

This is **Meta native edge detection**, not a Sobel compute shader. It does not
read camera textures. StereoKit still owns the OpenXR instance and session.

After changing branches:

```bash
cd android
./gradlew assembleDebug -PvoxenStereoKit=true
adb install -r app/build/outputs/apk/debug/app-debug.apk
```

Status: experimental, not built/tested on hardware from this environment.
If the build cannot locate `openxr/openxr.h`, ensure StereoKit's transitive
OpenXR include paths are exported to the Voxen target. For runtime diagnosis,
filter logs for `[Voxen]`. The Quest runtime may reject a second independent
passthrough layer or blend it differently; fallback should preserve the
transparent passthrough when native edge support is unavailable.


## Voxel brush prototype

Branch: `feature/stereokit-voxel-brush`. On Quest 3, use the **right controller trigger**
to paint cubes directly in space. The brush preview follows the right controller
aim pose (local -Z); use **Tip offset (m)** to adjust its distance. The slider
**Size (m)** controls new voxels only; existing voxels keep their original size.
The **Hue** slider changes the tint of newly painted voxels. Rotation is fixed
to world axes. **Undo last voxel** removes one voxel; **Clear voxels** removes all.

The renderer keeps at most 4096 cubes in memory and submits them each frame.
Stroke interpolation limits gaps during normal controller movement. This is
a proof of concept, not yet a batched GPU-instancing or persistence solution.
Current undo removes only one voxel, not an entire stroke. No sound or shader
selection is wired up yet.

```bash
cd android
./gradlew assembleDebug -PvoxenStereoKit=true
adb install -r app/build/outputs/apk/debug/app-debug.apk
```

**Unverified:** The changes have not been compiled or tested on Quest 3 in this
environment. The established passthrough controls remain intact.


## Carousel alignment and stroke undo experiment

Branch `feature/palette-alignment-stroke-undo`:
- Wider 34 cm translucent carousel, 47 cm high, 30 cm UI page centered on its front face.
- Controller anchor raised, leaving the UI lower edge closer to the cube bottom.
- Lighter opaque blue background (HSV value 0.33 instead of 0.12).
- Undo now removes the entire latest trigger stroke, not just the last voxel.
- Clear also resets the stroke history.

The four menu pages are still switched dynamically on a front-facing UI surface;
they are not four simultaneously mounted UI panels. The placement and height
are unverified on-device and may require further calibration.


## Right-controller ray interaction

The experimental panel now uses explicit row rectangles shared by the visible
StereoKit widgets and the right-controller ray intersection. Point the ray at a
button and press the right trigger once to activate it; press and hold on a
slider and move laterally to adjust. While the ray targets the panel, or while
a panel gesture is captured, voxel painting is suppressed. Outside the panel,
the trigger continues painting. X = undo stroke, Y = redo stroke.

This manual ray bridge is intentionally independent of StereoKit's near-hand
button interactions. Layout calibration and controller behavior still need
validation on Quest 3 before merging.


## Four-face tools experiment

- Four separate StereoKit windows are mounted on the rotating cube's face transforms.
- Left stick cycles through Colors, Shapes, Sounds, Scenes; panel rigid poses rotate with the cube.
- Smaller carousel: 17 cm wide, 23 cm high, placed close to the left hand.
- Right stick vertical increases/decreases the next voxel's edge length by 1 cm (1-12 cm).
- New voxel positions are snapped to a size-dependent grid; an AABB overlap check rejects occupied positions. Identical-size consecutive voxels touch without intersecting. Mixed sizes are overlap-safe but are not guaranteed gap-free at every transition.
- Shapes: Cube and Sphere selectable. Tetrahedron not yet implemented.
- Colors: Red, Yellow, Cyan and Purple first-pass buttons.
- Sound and Scene panels are visible placeholders; no fabricated Save/Load or audio action.
- X/Y remain stroke Undo/Redo. Controller ray/trigger actions are still experimental.
- Quest device compilation and face alignment not yet verified.


## Face-aware remote controls experiment

Branch `feature/vr-palette-ray-controls`:
- Right controller ray displays only when intersecting one of the four physical panel planes.
- Right trigger on the targeted face invokes manual UI hit testing; painting is inhibited for the duration of the UI press.
- COLOR face has a 4x4 HSV swatch palette.
- Right stick horizontal changes voxel size (+/- 1 cm per deflection);
  right stick vertical cycles Cube/Sphere.
- Brush cursor moved closer to the controller (2.5 cm from aim pose).
- Each face panel and its ray hit plane moved 15 mm right to address title drift.

Implementation uses custom ray hit testing instead of StereoKit hand proximity.
Not yet built or validated on Quest; face alignment and swatch targeting still
need hands-on testing.


## UI alignment, continuous resizing, hover and click sound

- Panel origins consistently offset to the right of each physical carousel face;
  the remote ray and UI visuals use the same face transform.
- Color swatches now use the same **local negative-X** orientation as the window,
  correcting swatches floating far off the carousel.
- Right stick X controls size continuously (1–12 cm); amount of deflection
  controls speed. Right stick Y still cycles shapes.
- Hovering color chips slightly enlarges them; hovering button rows shows a
  cyan translucent highlight.
- StereoKit synthesizes a short click tone and plays it when clicking a
  palette chip or button (no bundled audio dependency).

This is an experimental iteration and still requires Quest 3 build/device
validation for UI geometry, sound and interaction.
