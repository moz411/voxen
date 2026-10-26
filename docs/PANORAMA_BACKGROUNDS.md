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
- Vulkan texture upload and GPU lifetime management (CPU decode/validation implemented).
- Connect staged panorama shaders to the Vulkan pipeline and per-eye matrices; implement improved sky/ground reconstruction and edge handling.
- Scene serialization and gallery restoration of selected panorama.
- Headset test and build verification.

## Verification needed
- Android Gradle/NDK compilation, immersive SAF activity transition, import and rescan on a Quest 3.
- Selecting a filename now attempts bounded CPU JPEG/PNG decode and displays dimensions or an error; it does not activate the image yet.
- Panorama vertex/fragment GLSL shaders are added to the CMake shader compilation list, but no graphics pipeline or texture descriptors are bound yet.
- UI yaw and horizontal/vertical coverage controls are available but not yet connected to a GPU draw.

## Implementation caveat
Launching Android's 2D system picker from an immersive OpenXR session may require
session focus/pause handling and device-specific testing. This branch does not
claim that the complete import flow or panorama rendering works on Quest 3.
