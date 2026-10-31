# Video media engine — phase 1 (experimental)

Branch: `feature/video-media-engine`

## Current scope

`android/app/src/main/java/org/voxen/xr/media/VideoDecoder.java` is an isolated
hardware decoder prototype. It uses Android `MediaExtractor` +
`MediaCodec`, outputs video to a GPU-backed `ImageReader` with
`ImageFormat.PRIVATE` and `AHardwareBuffer.USAGE_GPU_SAMPLED_IMAGE`,
and exposes decoded frames as `android.media.Image`.

- Local video track only; video MIME selected from the container.
- Background codec thread, nonblocking frame dequeue and timestamp-based release.
- Playback, pause, seek and optional loop.
- No CPU readback / bitmap conversion.
- **Not yet instantiated by VoxenActivity or the native render loop.**
- **No Vulkan import or presentation yet; this does not display video.**
- **No audio decoding or A/V sync yet.**
- No headset build or playback verification has been performed.

## Required Vulkan bridge before integration

1. Obtain the newest `Image` from `acquireLatestFrame()`; preserve its
   lifetime until GPU sampling is complete. The caller owns and closes it.
2. Call `Image.getHardwareBuffer()` and import via Android hardware buffer
   Vulkan extension (verify format, feature flags, usage and external-memory
   support on the device first). Do not assume all PRIVATE allocations are
   importable or sampleable.
3. Query `vkGetAndroidHardwareBufferPropertiesANDROID`, configure Vulkan
   external format / YCbCr conversion as required, and import device memory.
4. Respect Android buffer acquire fences and Vulkan submission/completion
   synchronization. Do not close or reuse frames during GPU reads.
5. Reuse descriptor sets / imported resources where possible; limit outstanding
   frames. Avoid one allocation per frame when the decoder reuses its buffers.
6. Preserve the existing panorama and shader pipelines; connect the video
   frame as an *optional* texture input.

This decoder cannot be treated as zero-copy end-to-end until the bridge is
implemented and profiled.

## Integration / test checklist

- Run `cd android && ./gradlew assembleDebug`.
- Deploy with `adb install -r app/build/outputs/apk/debug/app-debug.apk`.
- Observe `adb logcat -s VoxenVideo:V AndroidRuntime:E`.
- Create an Activity/JNI owner with lifecycle matching active scene state.
- Start playback of a small H.264 MP4 in app-private storage, verify reported
  dimensions and sampled Vulkan frames on Quest 3.
- Test repeat, seek, pause/resume, scene exit, application background/foreground,
  unsupported codec, oversized resolution and full ImageReader queue.
- Profile GPU memory and latency for 1080p and 4K, and verify the decoder
  is hardware-accelerated on the device.

Do not merge to main before the JNI/Vulkan bridge and device tests.


## Vulkan import module (new, experimental)

`platform/android/src/AndroidVideoImage.cpp` now provides an **explicit,
non-rendering** import of an `AHardwareBuffer` as a Vulkan sampled image.
It creates dedicated imported memory, a YCbCr conversion, image view and
sampler, and owns their destruction. The Android `Image` and its native
buffer reference remain owned by `AndroidVideoDecoder::Frame`.

**Important:** creation of a VkImage is not evidence that its content is safe
for sampling. Before actual rendering, implement and validate:

- Codec/producer acquire fence semantics, Android buffer external queue-family
  ownership, and matching Vulkan barriers/layout transitions.
- GPU completion fences before `AndroidVideoDecoder::releaseFrame()` and
  `AndroidVideoImage::reset()`. Never destroy image/view while in flight.
- Descriptor-set lifetime and sampler-YCbCr immutable sampler requirements.
- Queue pacing/backpressure: max 3 outstanding ImageReader images; reused
  buffers and bounded Vulkan imported-image cache.
- Test decoder-produced `PRIVATE` buffers on Quest 3; Vulkan import can fail
  based on hardware/format/extension support.
- Validate Android SDK/NDK compilation and vendor runtime behavior.

Current behavior is **not yet active** in Voxen's render loop. No video playback
or image display has been validated on Quest 3.


## Opt-in MP4 → VkImage smoke test (latest)

The Android entrypoint probes exactly one decoded frame **only when** an
`internalDataPath/video-test.mp4` file exists and Vulkan external-video
support is enabled. This is an import/lifetime test; it does not display video
or submit GPU work. No MP4 file means the existing UI is unaffected.

To exercise on a debug APK with `run-as` allowed:

```bash
cd android && ./gradlew assembleDebug
adb install -r app/build/outputs/apk/debug/app-debug.apk
# Replace ./sample.mp4 with a short local H.264 video:
adb shell run-as org.voxen.xr mkdir -p files
adb shell run-as org.voxen.xr sh -c 'cat > files/video-test.mp4' < ./sample.mp4
adb shell am force-stop org.voxen.xr
adb shell monkey -p org.voxen.xr 1
adb logcat -v time -s VoxenVideo:V AndroidRuntime:E
```

Log outcome: `MP4 smoke-test armed`, `Decoder ready`,
`AHB ... sampleable=1`, `Imported video VkImage`,
`MP4 frame VkImage import: OK`.
These are *expected* messages, not yet verified on Quest 3.
If importing fails, capture the whole VoxenVideo log.

**Limitations:** no render pass, no external ownership-transfer barriers,
no decoder-acquire fence integration, no GPU completion fence, no audio. The
smoke test deliberately creates/imports/destroys only one image and issues
no GPU commands. Do not treat it as a video player.

## UI navigation update

- The `OBJECTS` carousel page contains two tabs: procedural `VOXENS` and imported `GLB MODELS`.
- The `BACKGROUNDS` page lists `.mp4` files from `/sdcard/Android/data/org.voxen.xr/files/Videos`, with a manual rescan command.
- Selecting a video currently changes only the UI state; it does **not** activate playback or projection. The standalone opt-in smoke test still uses `Videos/video-test.mp4`.
- Next milestone: a synchronized sampled-video image in a real renderer descriptor set, followed by panorama/wall projection.

## Unified media selection in carousel

The Backgrounds page no longer has media-type tabs. Two source selectors
list images and MP4 clips as peers (with an explicit [IMAGE]/[MP4] prefix):

- Background: passthrough, JPG/PNG panorama or MP4.
- Walls: shader, JPG/PNG or MP4.

This is **UI/state only** for MP4 and image-on-wall selection. The pre-existing
image-on-sphere path and wall shader renderer are unchanged. Selecting an MP4
currently does not display video: a GPU-synchronized Vulkan video texture
consumer must still be implemented. Never consider media selection a successful
playback test. These new selectors are not yet persisted in SavedRuntime.
