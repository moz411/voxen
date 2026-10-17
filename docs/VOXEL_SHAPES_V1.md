# Voxen — VOXELS / PARAMS prototype

Experimental branch `feature/voxel-shapes-presets-v1` from stable `main`. No merge until Quest validation.

## Controls
- Navigate to **VOXELS** (previously SHAPES). Select one of eight shapes: cube, sphere, cone, torus, cylinder, star, cross or diamond. Selection opens **PARAMS** automatically.
- PARAMS: brush voxel size (1–30 cm), RGB color and **NEON**, **ORGANIC**, **FRACTAL** material variants. Enable `Voxel brush enabled` here or in VOXELS.
- **Right-hand controller trigger** paints continuous sampled voxel stamps at the controller tip. Spacing is 65% of selected voxel size; maximum 2048 stamps for initial Quest tests. A live preview follows the right controller when it is not aiming at the UI.
- Trigger interaction with the UI is not treated as paint; disabling brush returns to the preexisting artwork grab behavior.
- **CLEAR VOXELS** erases all stamps. Each stamp retains selected shape, color, size and material at the time it was placed. No persistence across app launches in this prototype.

## Build and test
```sh
cd ~/voxen
git fetch origin
git switch feature/voxel-shapes-presets-v1
git pull --ff-only origin feature/voxel-shapes-presets-v1
cd android
./gradlew assembleDebug
adb install -r app/build/outputs/apk/debug/app-debug.apk
adb shell am force-stop org.voxen.xr
adb shell am start -n org.voxen.xr/android.app.NativeActivity
```

Test menu navigation, shape changes, voxel stamping, movement while pressing, UI/painter interaction, clear and stereo rendering. Check GPU with a few hundred stamps. Shape thumbnails, advanced per-voxel shaders, deformations, animation, sound and persistence remain subsequent iterations. This version has not yet been compiled/tested on Quest 3.
