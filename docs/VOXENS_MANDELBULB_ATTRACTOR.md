# Mandelbulb and Strange Attractor — Voxen experiments

The VOXENS preset combo now includes PLASMA, SPHERES INTERSECTIONS, MANDELBULB, STRANGE ATTRACTOR. Each painted instance retains its own preset.

**MANDELBULB:** Procedural power-8 triplex Mandelbulb based on the mathematical equation in https://www.skytopia.com/project/fractal/2mandelbulb.html. Fragment raymarching is capped at 32 samples / 6 fractal iterations for Quest performance. Visual detail is intentionally limited, and the projection uses a flat rendering proxy rather than a full view-dependent volume.

**STRANGE ATTRACTOR:** Adaptation of the formula published by Samuel (sml) in https://blenderartists.org/t/animated-strange-attractor-simulation/1557198 : `P += cos(P.zxy + T) - S*P`, `S = 0.05 .. 0.2`. Geometry generated in the vertex shader as 24 animated, spatial line-strands of 18 segments each; no imported Blender geometry. This is an interpretation of the published dynamical system, not an exact reproduction of the author's simulation.

Both effects reuse VOXENS size, hue, density and speed (density currently mostly relevant to the existing effects), the right-hand trigger brush, and the 256-instance limit. Existing Plasma and Spheres Intersections remain intact. No sound per Voxen or saved artwork yet.

Try with 1–5 copies first and check GPU timing before painting many Mandelbulbs: a full-screen raymarched fractal is expensive on Quest 3. Code pushed to branch `feature/voxens-plasma-v1`, not built or tested on-device by the assistant.

```sh
cd ~/voxen
git switch feature/voxens-plasma-v1
git pull --ff-only origin feature/voxens-plasma-v1
cd android && ./gradlew assembleDebug
adb install -r app/build/outputs/apk/debug/app-debug.apk
```
