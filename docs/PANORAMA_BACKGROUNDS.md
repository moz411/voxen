# Panorama backgrounds — integration status

Branch: `feature/panorama-backgrounds` (not merged into `main`).

## Implemented
- Android SAF document picker launched from **Backgrounds → Panoramas → IMPORT JPEG / PNG**.
- Copies selected files into `getFilesDir()/voxen-gallery/panoramas` without storage permissions.
- Native JNI polling and photo-library refresh; stable filename selection on rescan.
- JPEG/PNG validation and bounded RGBA8 decode (maximum 64 MiB encoded, 8192 px per axis, 33.5 million pixels).
- Reuses the existing Vulkan staging/upload code to allocate a sampled SRGB texture and descriptor set.
- Per-eye fullscreen panoramic draw before world objects, with OpenXR eye orientation and asymmetric FOV.
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
- Measure GPU memory use and frame time with 8K images.

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
- Import decoding and GPU upload occur on selection and may cause a short pause for large files.
