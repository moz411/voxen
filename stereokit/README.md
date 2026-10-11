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

## Unified four-face transform (Quest alignment experiment)

All four panel poses are now derived from a single `face_panel_pose(page)`
function. This defines the face rotation, top-right StereoKit window origin,
and a consistent 2 mm clearance above the cube face. Window rendering,
16-color swatch positions, and controller-ray intersections share these poses.
No independent magic offsets remain between panel drawing and ray targeting.
The cube stays upright; the left stick still rotates its four faces.

Not yet built or Quest-validated: verify title alignment, colors flush against
the panel, and right-trigger selection before merging into main.


## Cube-face UI rewrite

The four StereoKit floating windows have been removed. Their auto-sizing titles
and content were visibly detached from the rotating cube in Quest stereo
screenshots. Each face now has a self-contained fixed-size translucent plate,
title and controls rendered directly as meshes/text at known face-local
coordinates. Right ray hit testing uses exactly those face-center transforms.
No autogenerated window borders/headers remain. The ray only appears when
intersecting a face; swatches and shape buttons highlight on hover and click
with a short generated sound. Sound/Scene actions remain placeholders.

**Requires Quest 3 testing:** text facing direction, panel visibility, remote
click hitboxes, and depth order. Do not merge until verified on headset.


## Animated voxel painting and spatial notes — experimental

The SHAPES face keeps Cube/Sphere and adds an FX button cycling four presets:
SOLID, BREATHE, RAINBOW, SHIMMER. Each newly painted voxel retains its effect
preset; older strokes are unaffected by later preset changes. BREATHE reduces
mesh scale below the original collision/placement size, so it cannot overlap
neighboring voxels. RAINBOW and SHIMMER animate the color passed to the
StereoKit material at render time; **these are CPU-controlled effects, not yet
custom GPU shader programs**. The fixed grid and stroke Undo/Redo remain.

SOUNDS face now offers SILENT, BELL, PULSE and BASS. The three instruments are
synthesized on initialization with StereoKit sound_generate and are played
spatially at accepted voxel positions, throttled to at most one note per eight
placed voxels. UI click sound is unchanged. Sound presets apply to the next
painted voxels; no persistent audio sources or scene serialization yet.

Build/test on Quest 3 before merging. This branch is intentionally isolated
from main while we validate both audio routing and frame time with 4096 voxels.


## Five-panel procedural painting experiment

Branch `feature/algorithmic-brush-five-panels` (not yet Quest-tested):

- The four-face tool cube becomes a **five-faced carousel** with 72 degree
  left-stick steps and 46 cm panel height, twice the previous 23 cm.
- Unlike the earlier world-upright carousel, the entire tool rotates with
  left-controller orientation, including pitch and roll.
- Text is reduced to 8.5 mm; buttons to 25 mm, with compact selected-state
  highlighting. Sound and animation options are exposed individually, not
  hidden behind a cycling toggle.
- The fifth **ALGORITHMS** panel selects 10 brush-generation modes:
  FREE, JITTER, WAVE, HELIX, SHELL, LATTICE, MANDELBROT, JULIA, BRANCH, CORTEX.
  These are inexpensive *stylized procedural voxel placement samplers*.
  MANDELBROT/JULIA use 16 iteration escape-time samples to perturb height and
  voxel size; neither mode is a full fractal mesh generator. CORTEX is a
  folded-field effect, not a medical reconstruction.
- Every generated candidate is still tested against voxel AABBs to reject
  overlaps. All dimensions currently scale uniformly per voxel; independent
  X/Y/Z voxel scaling is future work.
- The 5 surfaces are rendered individually in a pentagonal arrangement,
  rather than inside a four-faced underlying mesh. Scene Save/Load remains
  a placeholder.
- Build with plain `cd android && ./gradlew assembleDebug` (StereoKit default).
  No install step documented. Validate orientation, page picking, buttons and
  performance on Quest 3 before merging.


## Revised algorithmic gesture / square carousel

- The carousel has four physical cube faces again. Five logical pages are
  available: the ALGORITHMS page takes the place of COLORS on its physical face
  when selected. Stick steps are 90 degrees, not five 72-degree wings.
- An active algorithm can be deselected by clicking its highlighted row again.
  With no algorithm selected (-1), the brush returns to normal freehand.
- Algorithmic painting is gesture-based: pressing the right trigger marks the
  first corner; moving the controller sizes a translucent world-aligned cube;
  releasing the trigger generates a discrete 3D structure *inside* that cubic
  volume, in one Undo stroke.
- The cube is centered between gesture endpoints. Its edge is the maximum of
  the three axis differences (with a minimum of three brush sizes), capped
  at 0.8 m for Quest testing. The generator samples at most 11³ positions
  and never exceeds 4096 total voxels.
- The ten algorithms apply different volumetric occupancy fields: sphere,
  jitter, wave, helix, shell, lattice, Mandelbrot and Julia height fields,
  branching and stylized cortex folds. Voxel sizes vary per sample but stay
  below lattice spacing; overlapping AABBs are rejected.
- This is a first bounded prototype, not a full volumetric Mandelbrot set.
- Build with `cd android && ./gradlew assembleDebug` (StereoKit is default).
  Not yet compiled or Quest-validated; leave main untouched.


### UI correction after Quest screenshot (single-face mode)

The 46 cm high five-face geometry was visibly unusable in the Quest: the
panels intersected and looked like a flower. The UI now displays **one**
controller-mounted page at a time. The left stick navigates all five pages;
only the selected page is rendered and ray-hit-tested. No translucent cube
shell or inactive faces are drawn. It follows the left controller's full pose,
with the tall panel center positioned 23.5 cm above the controller along its
local Y axis. This is an intentionally simpler, testable replacement for the
3D carousel while preserving all five page contents and the procedural brush.


## GPU voxel shaders — experimental StereoKit pipeline

Branch `feature/stereokit-gpu-voxel-shaders`, based on `main`.
- `stereokit/shaders/voxen_voxel.hlsl` is a real HLSL vertex/fragment shader compiled by StereoKit's `skshaderc_compile_headers` during CMake configuration/build into embedded shader data.
- Vertex stage passes **voxel-local coordinates** from the mesh to the fragment stage and applies StereoKit's instanced world and stereo view-projection matrices.
- Fragment stage implements six GPU surface looks: MATTE, NEON local grid, PLASMA waves, HOLO scanlines, LAVA flow and ICE cracks.
- COLORS panel selects materials in a 2x3 grid under the 16 color swatches; each new voxel stores its material index. Six StereoKit material instances share the shader; the time uniform is updated once per frame on each material.
- If shader initialization fails, rendering falls back to the previous default material. Existing scenes/undo/voxel geometry are not migrated.
- This experiment is independent from the separate `feature/surface-fractals-material-presets` branch.
- **No build or Quest validation performed**. Shader compiler integration, GLES shader variants, winding and performance must be validated on device before merging.

Build: `cd android && ./gradlew assembleDebug`. No APK installation command included.

## Attracteurs étranges — oeuvre 01 : Lorenz

This experimental branch adds **LORENZ** as the eleventh entry in the ALGORITHMS page.
The French exhibition title is **Attracteurs étranges**.

Unlike the earlier stylized occupancy algorithms, this uses the actual Lorenz
ordinary differential equations (sigma = 10, rho = 28, beta = 8/3), integrated
with fourth-order Runge–Kutta, dt = 0.008. We discard 1,500 initial warmup
steps, then sample 2,600 states, fit the trajectory uniformly into the cube
defined by trigger press/release, and place small voxels along that 3D curve.
The sculpture remains a single Undo stroke and uses the selected GPU material.
To avoid overwhelming the headset, the generator respects the existing total
voxel cap and emits at most one spatial note on completion.

This first stage uses fixed Lorenz parameters, with no per-parameter UI yet.
A future iteration can expose sigma/rho/beta, multiple starting states and
time-dependent drawing. This branch is **not yet built or Quest-tested**.
Build with `cd android && ./gradlew assembleDebug`.
