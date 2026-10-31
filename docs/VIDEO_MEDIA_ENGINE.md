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
