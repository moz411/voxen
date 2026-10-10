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
