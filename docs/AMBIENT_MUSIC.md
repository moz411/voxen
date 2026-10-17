# Voxen MUSIC panel (prototype)

Branch: feature/ambient-music-panel (based on feature/background-hepp-glow).

A ninth carousel page **MUSIC** uses native Android AAudio (PCM float stereo) for continuous, original procedural ambient loops. Choose Cosmos (low drone), Ocean (filtered surf), or Forest (airy texture and distant chirps), then PLAY/PAUSE and set volume.

No third-party tracks or music assets are included. The prototype uses realtime synthesis and repeats a 120-second phase cycle. The audio is non-spatial (stereo/dual-mono) for now; 3D sound and asset-backed tracks can be added later.

Quest test:

```bash
cd ~/voxen
git fetch origin
git switch feature/ambient-music-panel
git pull --ff-only
cd android
./gradlew assembleDebug
adb install -r app/build/outputs/apk/debug/app-debug.apk
adb shell am force-stop org.voxen.xr
adb shell am start -n org.voxen.xr/android.app.NativeActivity
```

In MUSIC, try each atmosphere, pause/resume, adjust volume, and check Quest headset audio and GPU. This code has not yet been compiled/tested on hardware. If opening AAudio fails, MUSIC displays 'Audio output unavailable'.

## Floor / ceiling backgrounds

Meta Scene now classifies bounded-2D anchors as WALL_FACE, FLOOR or CEILING. BACKGROUNDS > Floor & ceiling (enabled by default) projects the selected wall effect onto at most the nearest floor and the nearest ceiling in addition to Max walls. This includes Liquid Room, Solid Color, Matrix Rain and Hepp Glow. Pose and dimensions still come from 2D Meta anchors; static Scene Mesh depth occlusion remains optional. Use the toggle to compare and to reduce GPU load.
