# Voxen

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

## Panoramic backgrounds experiment

The BACKGROUNDS carousel page now renders an animated inside-facing sphere centered on the headset. It is independent of scene recognition; no room scan or spatial permission prompt is required. The default is OFF. The first entry, **CYBER FUJI DEMO**, is a *new procedural preview*, not the original Cyber Fuji 2020 shader (its source is not bundled in this repository). More environment shaders can be added to this registry once their source files are available.

**Compositor limitation:** this first implementation draws a translucent panorama in StereoKit's projection layer, which blends with passthrough behind that projection. It does **not** yet place a true environment OpenXR composition layer *under* the Meta passthrough layer; achieving strict environment -> passthrough -> voxels ordering requires separating projection content into composition layers. Test the visual layering and controls on Quest before merging.

## Godot Grass Wind material

`TEXTURES` includes `GRASS WIND` (material ID 9), adapted from a Godot spatial shader to StereoKit HLSL. The vertex stage performs per-voxel procedural vertical scaling and wind sway. The fragment stage blends green gradients and noise-based colors. The original Godot shader references three external noise textures not provided, so this port replaces them with deterministic procedural value noise. The interactive flatten position/radius, user-defined colors, and debug switches are not yet exposed. Test on Quest: no local APK build has been run for this change.
