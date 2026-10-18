# Mandelbulb volumetric prototype — stereo Quest 3

Dedicated experimental branch based on `feature/voxens-plasma-v1`. The other Voxens (Plasma, Spheres Intersections, Strange Attractor), BACKGROUNDS and MUSIC remain unchanged.

## What changed
- **MANDELBULB** is no longer drawn on a 2D quad. A cube (-1..+1) bounds the raymarch volume. This cube is only rendering geometry and is not an artistic mesh.
- Every XR eye supplies its world-space pose, converted into local coordinates of each Mandelbulb (translation and uniform scale).
- A fragment shader intersects the view ray with the cube, raymarches Mandelbulb inside the interval and shades the distance-estimator surface. The correct projected depth of the fractal hit is written to `gl_FragDepth`.
- Push constant range extended from 80 to 96 bytes only for the volumetric pipeline; existing pipelines still push 80 bytes.
- Stereo parallax should now change naturally when viewing a Mandelbulb from the side. This still uses a power-8 fractal approximation with limited ray/DE iterations for mobile GPU.

## Performance and test
The volume proxy is twelve triangles, with fragment raymarch cost up to 36 steps and six Mandelbulb iterations per step, with additional normal estimation. Try **one instance** first, and check GPU frame duration. Backfaces and other overlapping Voxens can increase fill cost. The generic cap of 256 painted instances is not a performance guarantee for this preset.

```sh
cd ~/voxen
git fetch origin
git switch feature/mandelbulb-volumetric-v1
git pull --ff-only origin feature/mandelbulb-volumetric-v1
cd android && ./gradlew assembleDebug
adb install -r app/build/outputs/apk/debug/app-debug.apk
```

In VOXENS, select MANDELBULB and paint one copy. Walk around it and view from the side: unlike the prior quad, it should retain 3D perspective. Verify no incorrect black cube, double-eye artifacts, or frame-rate collapse. If a shader compilation error or Vulkan validation error appears, send the first offending log entry.

**Not yet compiled or tested in the Quest headset by the assistant.** No changes merged to main.
