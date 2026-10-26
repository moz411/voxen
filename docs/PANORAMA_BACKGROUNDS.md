# Panorama backgrounds — development status

Branch: `feature/panorama-backgrounds`.

## Implemented in this branch
- Android NativeActivity subclass with a system SAF image picker.
- JPEG/PNG selection and copy to app-private `files/voxen-gallery/panoramas`.
- A 64 MiB file-size cap, unique filenames, and a synchronized queue of imported paths.
- `openPanoramaPicker()` and `pollImportedPanorama()` methods for a future JNI UI bridge.
- No broad external-storage permission required.
- Native XR loop polls completed imports through JNI and logs their private paths (no decode/render yet).

## Not yet implemented
- Calling the picker from the native Backgrounds carousel.
- Passing polled paths from the JNI bridge into the renderer's validated texture loader.
- JPEG/PNG decoding, validation, texture upload, and GPU lifetime management.
- Cylindrical/equirectangular shader, seamless edges, floor/sky fill, yaw control.
- Scene serialization and gallery restoration of selected panorama.
- Headset test and build verification.

## Implementation caveat
Launching Android's 2D system picker from an immersive OpenXR session may require
session focus/pause handling and device-specific testing. This branch does not
claim that the complete import flow or panorama rendering works on Quest 3.
