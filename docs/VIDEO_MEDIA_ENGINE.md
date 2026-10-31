# Voxen video playback — OpenXR Android Surface (experimental)

Branch: `feature/video-media-engine` / draft PR #51.

## Architecture

```
Backgrounds [MP4] → selectedBackgroundVideo()
    → XrSession::openVideoSurface()
    → xrCreateSwapchainAndroidSurfaceKHR()
    → Android MediaPlayer.setSurface(surface)
    → XrCompositionLayerEquirect2KHR in xrEndFrame()
```

The Quest runtime compositor samples the decoder's Surface directly. There is no
Vulkan sampling, YCbCr sampler setup, CPU frame extraction, RGBA upload, or
per-frame queue/device idle in the video path.

Enabled only when **both** `XR_KHR_android_surface_swapchain` and
`XR_KHR_composition_layer_equirect2` are advertised by the installed runtime.
No extension is assumed present. If absent, image/shader rendering remains
available and MP4 playback is disabled.

## Functional scope

- One global 360° equirectangular video layer, centered in LOCAL space,
  mapped to an infinite sphere.
- Android MediaPlayer hardware decode when a supported codec is available.
- Automatic looping and track audio as handled by MediaPlayer.
- On selection change: stop the previous player, destroy the old Surface
  swapchain, create the new one and start the new MP4.
- On XR STOPPING: stop producing frames **before** `xrEndSession`.
- On XR READY after stopping: re-open the selected video when running.
- **Not yet implemented:** projecting videos onto individual Meta walls,
  multiple parallel video sources, audio mixing with the existing music engine,
  file-specific dimension/orientation mapping, seeking and playback controls.

## Cleanup

Removed from Android build and repository:
- MediaCodec → ImageReader → AHardwareBuffer experimental decoder and
  JNI wrapper.
- `AndroidVideoBuffer`, `AndroidVideoImage`,
  `VideoFrameDescriptor` and related CMake references.
- Synchronous `MediaMetadataRetriever` 512×256 RGBA preview and its
  per-frame Vulkan upload path (the source of XR stalls).
- Obsolete video-preview methods on `XrSwapchainRenderer`.

JPEG/PNG panorama uploads remain unchanged.

## Test on Meta Quest 3

```bash
cd ~/voxen
git pull --ff-only origin feature/video-media-engine
cd android
./gradlew assembleDebug &&
adb install -r app/build/outputs/apk/debug/app-debug.apk

adb logcat -v time -s VoxenVideo:V Voxen:V AndroidRuntime:E
```

With an MP4 in `Android/data/org.voxen.xr/files/Videos/`, choose its
`[MP4]` entry in Backgrounds and set Apply to `GLOBAL SPHERE`.

Expected logs:
- `OpenXR Android Surface + Equirect2: enabled`
- `OpenXR Android Surface player preparing: ...`
- `OpenXR Surface playback started: ...`
- `OpenXR Surface playback: STARTED (result=0)`

If `unavailable`, collect the runtime extensions; if `FAILED`,
capture the XrResult and the Java exception from Logcat.

**Unverified**: Android build and on-device playback have not been tested
after this refactor. The OpenXR runtime and actual MP4 codec support must
be validated on the headset. Keep PR #51 in draft until validated.

## Reference

- Khronos `XR_KHR_android_surface_swapchain` spec:
  https://registry.khronos.org/OpenXR/specs/1.1/man/html/xrCreateSwapchainAndroidSurfaceKHR.html
- Khronos `XrCompositionLayerEquirect2KHR`:
  https://registry.khronos.org/OpenXR/specs/1.1/man/html/XrCompositionLayerEquirect2KHR.html
- Khronos runtime inventory indicates Quest 3 support, but the
  installed firmware is authoritative.
