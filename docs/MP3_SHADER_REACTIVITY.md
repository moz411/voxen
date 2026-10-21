# MP3 → Voxen shader modulation (design, 2026-10-21)

## Source and constraints
- Source is the **MP3 currently played in the MUSIC panel**, not the headset microphone or environmental sound.
- Current playback: `Mp3Library` wraps Android `MediaPlayer` via JNI, loops and controls gain.
- `MediaPlayer` does **not expose decoded PCM** through this JNI integration. Do not synthesize fake spectrum from playback position, and do not assume `Visualizer` can capture playback without additional platform constraints/permissions.
- Preferred implementation: own the decoded PCM via Android `MediaExtractor` + `MediaCodec`, feeding an `AAudio` output stream and a lock-free analysis ring buffer. Retain existing MediaPlayer until replacement is validated in headset.

## Pipeline
```
MP3 file -> MediaExtractor -> MediaCodec -> float PCM -> SPSC audio FIFO -> AAudio callback -> speakers
                                                |
                                                +-> bounded SPSC analysis FIFO -> analysis worker -> atomic modulation snapshot
                                                                                   |
XR frame -> snapshot once -> smoothing/mapping -> Vulkan push constants / UBO -> voxel shaders
```
- Audio callback: no allocation, locks, JNI, logging, FFT or disk I/O.
- Decode and analysis run off audio callback; timestamp PCM frames in sample time, not wall clock.
- Analysis: mono mixdown; 2048 or 4096-sample Hann FFT, 50-75% overlap; sample-rate-aware frequency bins.
- Normalize via adaptive envelope with noise floor and headroom; low (20-250 Hz), mid (250-4000 Hz), high (4000-min(16000,Nyquist) Hz), RMS, spectral flux/onset.
- Attack ~20-40 ms, release ~150-300 ms; clamp 0..1; hysteresis and minimum beat interval ~180 ms.
- Publish immutable `AudioFeatures {low,mid,high,rms,onset,beatPhase,sequence,playing}` as an atomic snapshot or double-buffer protocol with proper synchronization; never read a partially written struct.
- When paused, stopped or missing audio: decay features to zero; no residual pulses.
- Volume slider controls playback gain, not analysis sensitivity; normalized features should remain stable at low volume.
- At MP3 selection switch or seek: reset FFT/history, stale beat state and timing.
- Avoid a per-voxel FFT or GPU readback: compute one feature set per frame and share it.

## Shader integration
- Preserve current push constant ABI where possible; use a shared per-frame UBO if adding six floats would exceed existing push constant layout or break pipelines.
- Keep existing `time/speed/hue/deformation` behavior when audio reactivity is off.
- Suggested per-material bindings:
  - Organic Sphere: low -> radius/deformation; mid -> surface ripples; high -> emissive detail.
  - Jellyfish: low -> bell pulse; mid -> tentacle motion; high -> bioluminescence.
  - Torus Knot: low -> thickness; mid -> twist; high -> sparkle.
  - Spike Bloom: onset -> expansion impulse; high -> spike glow.
  - Mandelbulb: prioritize low-cost palette/emission modulation; **do not** increase raymarch steps or fractal iterations on beats.
- Controls in MUSIC or PARAMS: Audio reactive ON/OFF, intensity (0..2), smoothing, per-band weights and material-specific mapping; OFF is deterministic baseline.

## Implementation sequence
1. Add `AudioFeatures` and analysis unit tests using generated sine sweeps, impulses, silence and changing sample rates.
2. Build an Android decoder/output backend with robust MP3 format changes, loop boundary, underrun and lifecycle handling; retain fallback.
3. Add real-time-safe analysis worker and lock-free handoff.
4. Integrate feature snapshot into XR renderer once per stereo frame and add conservative shader mappings.
5. Add UI controls and a small debug band meter.
6. Test Quest 3 latency, loop discontinuities, frame time and CPU overhead; keep 72/90Hz frame budget and disable expensive jellyfish collision modes.

## Acceptance criteria
- Same MP3 audio as before, no microphone permission.
- Visible bass pulse, mid movement, high shimmer, transient flash; no activity on pause.
- Both eyes receive identical audio features in one XR frame.
- No new audio-thread blocking, and no material frame-time regression when reactivity disabled.
- Existing MP3 selection, loop, pause, volume and lifecycle continue to work.
