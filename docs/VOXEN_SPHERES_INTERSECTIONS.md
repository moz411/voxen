# SPHERES INTERSECTIONS — Voxen preset (Quest prototype)

Reference: Higgsas, **Spheres Intersections**, BlenderArtists post #408 (27 February 2024):
https://blenderartists.org/t/higgsas-geometry-nodes-groups-blender-3-3-5-0/1351737/408

The user provided the original `spheres intersection.blend` (Blender 4.0 file, distributed with Zstandard compression). The Blender screenshot shows point distribution over a grid, random sphere radii, smooth geometry and a **Spheres Intersections** node group with relaxation iterations. The implementation here is **original GLSL procedural approximation**: a small grid of variable-radius spheres with slight animated offset and SDF smooth-union blending, raymarched in the fragment shader. It is **not an exact port of the Blender Geometry Nodes graph** and does not embed the .blend or redistribute author assets.

To try: select `VOXENS` -> `Voxen preset` -> `SPHERES INTERSECTIONS`, adjust `Size`, `Hue`, `Density` and `Speed`, then press the right trigger to paint. Previously placed Plasma Voxens keep their shader. This is rendered on a small, transparent flat proxy; view-angle-correct volumetric Voxens and sound/deformation will be subsequent iterations.

Performance: 3×3 spheres and up to 32 raymarch steps per fragment; total Voxen stamp cap remains 256. Start with a few instances and watch GPU time. Build/Quest compatibility **not yet independently tested**.

```sh
cd ~/voxen
git switch feature/voxens-plasma-v1
git pull --ff-only origin feature/voxens-plasma-v1
cd android && ./gradlew assembleDebug
adb install -r app/build/outputs/apk/debug/app-debug.apk
```
