# Scene Mesh static occlusion + Matrix Rain (experimental)

This branch starts from main and keeps the validated wall shader and wireframe modes intact.

## BACKGROUNDS menu
- Wall shader: LIQUID ROOM / SOLID COLOR / MATRIX RAIN
- Occlude walls with scene mesh: optional static geometry depth prepass, **off by default**
- Scene mesh wireframe: independent visual diagnostic

## How it works
The already validated Meta triangle mesh is uploaded once as triangle-list geometry alongside the existing wireframe edges. Before drawing wall effects, Vulkan renders the mesh with depth writes ON, color writes OFF. Wall shaders depth-test against those pixels: foreground furniture can hide wall effects without drawing opaque surfaces over passthrough. Only the first captured room mesh is used. The captured mesh is static, not live depth; moved objects won't occlude accurately.

Matrix Rain adapts the user-supplied Shadertoy green-rain/rotating flower GLSL to the project's local-wall UVs, time and opacity push constants. It shares the same tracked wall rendering path as Liquid Room.

## Quest test
```bash
cd ~/voxen
git fetch origin
git switch feature/scene-depth-occlusion-matrix
git pull --ff-only
cd android
./gradlew assembleDebug
adb install -r app/build/outputs/apk/debug/app-debug.apk
adb shell am force-stop org.voxen.xr
adb logcat -c
adb shell am start -n org.voxen.xr/android.app.NativeActivity
adb logcat -d -s VoxenScene:I AndroidRuntime:E
```

1. BACKGROUNDS: SOLID COLOR, opacity around 0.6, wall budget 8; look across a table and a wall.
2. Toggle **Occlude walls with scene mesh** and compare whether the table masks colored wall fragments.
3. Try MATRIX RAIN with opacity around 0.4; compare with Liquid Room.
4. Turn Scene mesh wireframe ON only to verify overlay registration; turn it off to judge occlusion.
5. Report GPU ms and stereo screenshot, particularly at wall/object boundaries.

## Limitations
This is the first depth-only pass; actual alignments and depth fighting require headset validation. No Meta Environment Depth, no dynamic occlusion, no window-aware cutouts, no fusion until tested.
