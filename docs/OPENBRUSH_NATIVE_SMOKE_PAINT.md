# Interactive Open Brush Smoke painting (experimental)

Branch `feature/openbrush-native-smoke-paint` replaces the seven synthetic
PARTICLES presets with one supported tool: **SMOKE (Open Brush)**. The
Open Brush Smoke GUID is `70d79cca-b159-4f35-990c-f02193947fe8`.

In edit mode, hold the right trigger and move the right controller to draw.
Samples are spaced by movement distance (at least 8 mm or 12% of brush size),
not frame count. Releasing the trigger finishes the gesture. Brush size and hue
use the existing controller stick / editor controls. Each sample is saved as a
static `SavedVoxen` with the reserved preset 27, allowing round trips through
the existing `VOXEN_RUNTIME 7` format without breaking older scenes. Only
the active viewport converts these samples to Vulkan quad geometry.

The original Smoke MainTex is installed with
`bash tools/install_openbrush_smoke.sh`. If absent, Voxen logs a warning and
uses the existing diagnostic substitute. The original `VXMSH002` importer and
its independent Smoke draw pass are unchanged.

Legacy pseudo-emitter preset IDs 20–26 remain decodable, but are not painted,
previewed, simulated or exposed in the PARTICLES catalog. They are preserved
in the scene payload for forward migration; they no longer produce visuals.

## Host tests

```
c++ -std=c++17 -Iengine/render/include tests/openbrush_smoke_stroke.cpp -o /tmp/voxen-smoke-stroke
/tmp/voxen-smoke-stroke
```

## Known limitations

- This is a first native brush integration, not an exact reproduction of
  Open Brush's stroke emitter logic. Point placement and alpha quads match
  the existing VXMSH002 Smoke shader semantics; actual stroke-specific
  pressure, tilt/orientation and temporal per-particle metadata are not yet
  serialized in runtime v7.
- The Vulkan Smoke shader still uses an approximate trigonometric curl flow,
  not Icosa's simplex curl implementation, and has no depth sorting.
- Dirty brush geometry is rebuilt with a GPU idle barrier; profile on Quest 3
  before dense strokes. The existing scene limit is 4096 objects, including
  Smoke samples. No undo grouping is implemented.
- Android build, Vulkan validation and optical comparison need physical
  headset testing.
