# MUSIC — local Quest MP3 playback

Voxen MUSIC now has two sources:
- PROCEDURAL: original COSMOS, OCEAN and FOREST AAudio ambient sound.
- MP3 FILES: Android MediaPlayer decoder, automatic loop, select file, PLAY/PAUSE, volume and RESCAN MUSIC FOLDER.

Copy files to the package-specific external directory (case-sensitive):
```sh
adb shell mkdir -p /sdcard/Android/data/org.voxen.xr/files/Music
adb push ~/Music/voxen/. /sdcard/Android/data/org.voxen.xr/files/Music/
adb shell ls -lh /sdcard/Android/data/org.voxen.xr/files/Music/
```

Supported selection: .mp3 extensions, case insensitive; filename sorted. No network required. File decode relies on Android MediaPlayer; invalid/corrupt tracks may report an error. On uninstallation, app-specific data may be removed; `adb install -r` normally keeps it.

Build/test:
```sh
cd ~/voxen && git fetch origin && git switch feature/ambient-music-panel && git pull --ff-only
cd android && ./gradlew assembleDebug
adb install -r app/build/outputs/apk/debug/app-debug.apk
adb shell am force-stop org.voxen.xr
adb shell am start -n org.voxen.xr/android.app.NativeActivity
```

The feature was implemented but has not yet been compiled or tested on Quest 3 by the assistant. Test MP3 discovery, loop after the end of a short track, source switching, pause, volume, and rescan. For diagnostics: `adb logcat -d -s AndroidRuntime:E Voxen:E`.
