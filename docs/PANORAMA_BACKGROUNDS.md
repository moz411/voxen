# Panorama backgrounds — integration status

Branch: `feature/panorama-backgrounds` (not merged into `main`).

## Implemented
- One ADB-accessible folder for panoramas, matching the existing MP3 workflow:
  `/sdcard/Android/data/org.voxen.xr/files/Backgrounds/`.
- Backgrounds → Panoramas → **RESCAN PHOTOS** lists JPEG/PNG files in this folder.
- No headset file picker required; the older Android picker plumbing remains dormant for now.
- Existing MP3 folder is `/sdcard/Android/data/org.voxen.xr/files/Music/`.
- JPEG/PNG validation and automatic aspect-preserving downscaling: up to 64 MiB encoded, up to 64 megapixels in source dimensions; GPU target capped to 16 megapixels / 64 MiB RGBA8 and the Vulkan device's maximum image dimension (also capped at 8192).
- Only one panorama texture remains resident after replacement. Source decode is currently full resolution in CPU memory before downscaling; extremely large images above 64 MP are rejected to avoid memory spikes.
- Reuses the existing Vulkan staging/upload code to allocate a sampled SRGB texture and descriptor set.
- Per-eye fullscreen panoramic draw before world objects, with OpenXR eye orientation and asymmetric FOV.
- JPEG decode and bilinear downscale run in one background worker, polled without blocking the XR frame loop. A selected panorama starts on opaque white and fades in over 1.5 seconds after upload.
- Outside partial panoramas, the background remains pure white with no edge feathering.
- Saved music playback choice is restored on scene load; gallery immersive entry already restores this choice.
- Adjustable yaw, horizontal coverage and vertical coverage for incomplete smartphone panoramas.
- Opaque background behind voxens and GLB; **USE PASSTHROUGH SOBEL** resets the selection.
- Scene snapshot format `VOXEN_RUNTIME 4` stores selected filename and angles; v1–v3 remain readable.
- Releases panorama texture on deselection/shutdown, and the graphics pipeline on shutdown.

## Testing still required
- **Not compiled or tested on Quest 3.** The GitHub gallery smoke workflow does not build the Android application.
- Verify Java NativeActivity subclass starts correctly and Android SAF picker returns focus to immersive OpenXR.
- Verify both eyes display a correctly oriented panorama, with no mirrored view, reversed vertical axis or stereo seam.
- Verify imported file persists across app restarts and scene Save/Load.
- Verify GLB/voxens render over the panorama, and that passthrough returns on deselection.
- Measure GPU memory use and frame time with 8K images; verify automatic downscaling of 10K–16K-wide JPEGs.

## Build and headset smoke test
```bash
git fetch origin feature/panorama-backgrounds
git switch feature/panorama-backgrounds
cd android
./gradlew assembleDebug
adb install -r app/build/outputs/apk/debug/app-debug.apk
adb logcat -s VoxenPanorama:* Voxen:* AndroidRuntime:E
```

## Caveats
- Fullscreen projection currently fills unknown areas with a neutral dark color, not a reconstructed sky or ground.
- Photos are displayed as monoscopic imagery projected independently into the two eye views, not as stereo 3D panoramas.
- A panorama overlays passthrough in the projection layer; it does not remove the underlying Meta passthrough composition layer.
- GPU staging/upload and vkDeviceWaitIdle still occur on the render thread when decode completes; this can cause a short frame hitch. Fully asynchronous GPU transfers require a dedicated command pool/queue and synchronization.

## Upload panoramas with ADB

```bash
adb shell mkdir -p /sdcard/Android/data/org.voxen.xr/files/Backgrounds
adb push panorama.jpg /sdcard/Android/data/org.voxen.xr/files/Backgrounds/
adb push ~/Images/Panoramas/. /sdcard/Android/data/org.voxen.xr/files/Backgrounds/
adb shell ls -lh /sdcard/Android/data/org.voxen.xr/files/Backgrounds/
```

Open Voxen → Edit → Backgrounds → Panoramas → RESCAN PHOTOS, then select a photo.
