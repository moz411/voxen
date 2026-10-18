# Voxen procedural system — Plasma v1

Experimental branch from main. **Not compiled on Android or validated in Quest 3 yet**.

## Concept

A Voxen is a time-evolving procedural audiovisual entity, not a conventional modeled mesh with a surface texture. The first Plasma preset renders a GPU-generated, animated energy field on a small transparent carrier quad (a rendering proxy, not the artistic geometry). This is a **2.5D visual prototype**, not yet a full volumetric raymarched entity; stereo inspection from the side will reveal that limitation. The next iteration should provide a camera-facing proxy or true view-dependent volume.

## Controls

The simplified carousel has three panels only: **VOXENS**, **BACKGROUNDS**, **MUSIC**. BACKGROUNDS and MUSIC remain available from main; obsolete default Neon/Organic/Fractal artworks are not drawn or compiled into render pipelines.

VOXENS currently contains one procedural preset: **PLASMA**.

- Paint Voxens enabled by default.
- Size 0.06–0.60 m; hue; energy density; animation speed.
- Right trigger deposits Plasma instances at the controller's forward tip (17 cm from controller origin), with spacing proportional to preset size.
- A live Plasma preview appears at the brush tip while not pointing at the UI.
- Each deposited Plasma retains its own size, hue, density and speed; maximum 256 instances in this first performance-limited test.
- CLEAR VOXENS removes those instances. No file persistence yet.
- Right-hand green UI ray is visible only when aiming at the carousel.
- Plasma does **not** have independent sound synthesis, deformable volume or spatial trajectory animation yet.

## Test on Chromebook / Quest 3

```sh
cd ~/voxen
git fetch origin
git switch feature/voxens-plasma-v1
git pull --ff-only
cd android
./gradlew assembleDebug
adb install -r app/build/outputs/apk/debug/app-debug.apk
```

In headset verify: (1) VOXENS page visible, (2) Plasma follows controller and triggers painting away from UI, (3) the panel does not paint into itself, (4) slider changes affect new Plasma and preserve older instances, (5) CLEAR works, (6) passthrough Sobel, BACKGROUNDS and MUSIC remain stable, (7) stereo visibility and FPS. If build fails, share the first C/C++ error.

Use only test textures, shaders, and music with rights appropriate for your eventual distribution.
