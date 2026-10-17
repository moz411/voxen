# Voxen

## Android renderer selection (Liquid Room experiment)

The Android Gradle build now selects **native OpenXR + Vulkan** by default on `feature/mr-layered-environments`. This path renders an experimental, synthetic Liquid Room quad in front of the user and preserves Meta passthrough. It does **not** yet detect room walls or implement the StereoKit carousel.

```bash
cd android
./gradlew assembleDebug
adb install -r app/build/outputs/apk/debug/app-debug.apk
adb logcat -s Voxen:I VoxenPerf:I AndroidRuntime:E
```

To build the older StereoKit application with its carousel instead:
```bash
./gradlew assembleDebug -PvoxenStereoKit=true
```

Because the two backends share the same Android package name, installing either APK replaces the other. This change has not been compiled or tested on the Quest hardware in this environment. The latest GPU timestamp output is reported through `VoxenPerf`, not a native head-locked HUD.


C++20 + OpenXR + Vulkan. Builds are **local on Windows**; no GitHub Actions.

## Quest 3 / Android APK

Open the `android/` folder in Android Studio. Install SDK API 35, NDK 27, CMake 3.22.1 from SDK Manager. Use **Build > Build APK(s)** (Debug) or run:

```powershell
.\scripts\build-android.ps1
```

The script uses `gradlew.bat` when `gradle/wrapper/gradle-wrapper.jar` exists, otherwise requires a `gradle` executable on PATH. **This repository currently does not contain the Gradle wrapper JAR.** Android Studio's integrated Gradle build is therefore the simplest path.

Output: `android/app/build/outputs/apk/debug/app-debug.apk`.

Install:

```powershell
adb install -r android\app\build\outputs\apk\debug\app-debug.apk
```

The Quest build enables native blue `XR_FB_passthrough` with white contours if the runtime supports that extension. No Vulkan camera Sobel shader is used.

## Windows x64 EXE

Requires:
- Visual Studio 2022 **Desktop development with C++** (MSVC toolchain and Windows SDK); Android Studio alone does not install MSVC.
- Vulkan SDK with `glslc.exe` and `VULKAN_SDK` environment variable.
- CMake 3.22+ on PATH (the Android Studio SDK CMake executable is also suitable).
- Khronos OpenXR SDK headers and `openxr_loader.lib`, built/installed for Windows x64. Supply its installation path.
- An active Windows OpenXR runtime that supports Vulkan (e.g. Meta XR Simulator).

Run from repository root in PowerShell:

```powershell
.\scripts\build-windows.ps1 -OpenXRRoot "C:\SDK\OpenXR" -Configuration Release
```

Output: `build/windows/Release/voxen_windows.exe`.

On Windows the OpenXR passthrough extension is not requested. The scene uses the existing opaque background; passthrough camera processing is Quest-specific.

## Source layout

- `engine/core`: engine lifecycle
- `engine/xr`: OpenXR sessions and passthrough
- `engine/render`: Vulkan swapchains, renderer and shaders
- `platform/android`: Android entry point
- `platform/windows`: Windows entry point
- `scripts`: local build scripts

## Experimental Scene backgrounds (`feature/scene-background-shaders`)

The StereoKit backend includes a **BACKGROUNDS** carousel page. It loads Meta Scene Model 2D planes through `XR_FB_scene` + `XR_FB_spatial_entity_query`, categorizes `WALL_FACE`, `FLOOR`, `CEILING`, and overlays one of the existing GPU materials on the detected surfaces. Default: **SHADER: NONE**. The Meta passthrough layer and its white edges remain independent of scene geometry. Re-scanning and per-surface category toggles are available from this page.

On Quest, enable Space Setup for the room and grant Voxen spatial data access (`USE_SCENE`). If there are no planes, check the `[Voxen Scene]` log lines. Scene detection is device-dependent; this branch has not been built or tested on a Quest yet. The native OpenXR engine under `engine/` is unchanged; this experimental integration targets the StereoKit app under `stereokit/`.

## Lightweight panoramic skies / animations (experimental)

Branch based directly on main. BACKGROUNDS contains OFF, AURORA and EMBER HORIZON. Both are single-pass procedural 360-degree shaders with a constant number of arithmetic operations, no texture lookups, no noise octaves, no particle loops, no scene scans. Only one sky material and one sphere are rendered. They are disabled by default and the opacity is capped at 60%.

ANIMATIONS now has eight modes: SOLID, BREATHE, RAINBOW, SHIMMER, SLOW PULSE, SOFT TIDE, RIPPLE, ORGANIC. These use bounded sine calculations per voxel and do not spawn extra objects. The old scene-overlay code is not initialized; main is unchanged. OpenXR compositor layer ordering is not yet refactored; skies are alpha-blended in the StereoKit projection layer rather than guaranteed strictly behind passthrough.

**No Android APK build or Quest GPU profile was run in this environment.** Test with device GPU timing before merging. The earlier expensive Fireworks code is deliberately excluded.

### Additional lightweight panoramas

Added GALAXY (analytic Milky Way-style glow and sparse stars), AQUATIC (underwater teal illumination and caustic ripples), and RAINFOREST (layered green canopy, silhouettes, light shafts and mist). BACKGROUNDS now cycles among five skies with PREVIOUS/NEXT and intensity controls. Every effect uses a fixed number of ALU operations and a single sphere mesh; none uses procedural particle iteration or sampling textures. The backgrounds remain OFF at startup. Their silhouettes are stylized, not textured photoreal environments. Performance and visuals still need verification on Meta Quest 3.

### Geometry animations (latest)

The ANIMATIONS menu now contains NONE, BREATHE, ROTATE, PINCH H, PINCH V and SPLIT 4. BREATHE changes uniform voxel size; ROTATE revolves each artwork around its shared center; PINCH H narrows the mid-height of an artwork along X; PINCH V compresses the middle width along Y; SPLIT 4 offsets the four XY quadrants outward and then brings them together. The deformation affects the rendered voxel transforms only, preserving unmodified stored artwork/placement geometry. For visible transformations, use a multi-voxel artwork (not a solitary voxel). These are procedural CPU transform animations with no additional geometry and no shader particle loops. New animations apply to strokes/works created with the selected animation, in line with existing app behavior. APK build and headset QA still required.

### MATRIX RAIN panoramic background

Sixth background preset: green falling pseudo-glyph columns with varied speeds, lit heads and fading tails. Generated entirely from simple procedural strokes in a single fragment pass. No font atlas, texture fetches, particles, geometry duplication, or per-pixel iteration. This is stylized pseudo-writing, not readable Unicode text. OFF on launch, governed by the same opacity control. Android compilation and Quest GPU timing remain to be tested.
