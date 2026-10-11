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
