# Panorama backgrounds — development status

Branch: `feature/panorama-backgrounds`.

## Implemented in this branch
- Android NativeActivity subclass with a system SAF image picker.
- JPEG/PNG selection and copy to app-private `files/voxen-gallery/panoramas`.
- A 64 MiB file-size cap, unique filenames, and a synchronized queue of imported paths.
- `openPanoramaPicker()` and `pollImportedPanorama()` methods for a future JNI UI bridge.
- No broad external-storage permission required.
- Native XR loop polls completed imports through JNI and refreshes the local image catalog.
- Backgrounds carousel exposes IMPORT JPEG / PNG, RESCAN PHOTOS and an image filename list.
- UI import action launches Android document picker through the native activity JNI bridge.

## Not yet implemented
- Passing polled paths from the JNI bridge into the renderer's validated texture loader.
- JPEG/PNG decoding, validation, texture upload, and GPU lifetime management.
- Cylindrical/equirectangular shader, seamless edges, floor/sky fill, yaw control.
- Scene serialization and gallery restoration of selected panorama.
- Headset test and build verification.

## Verification needed
- Android Gradle/NDK compilation, immersive SAF activity transition, import and rescan on a Quest 3.
- The image list is intentionally filename-only; selecting it does not activate an image yet.

## Implementation caveat
Launching Android's 2D system picker from an immersive OpenXR session may require
session focus/pause handling and device-specific testing. This branch does not
claim that the complete import flow or panorama rendering works on Quest 3.
