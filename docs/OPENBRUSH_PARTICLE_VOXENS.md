# Open Brush-inspired particle Voxens — implementation plan

Branch: `feature/openbrush-particle-voxens`. Status: design only; no particle rendering implemented.

## UI
Add a third tab **PARTICLES** in OBJECTS alongside VOXENS and GLB MODELS.
Initial presets: SMOKE, SNOW, STARS. The selected particle Voxen should use the existing right-controller preview, right-trigger placement, resize, clear and scene persistence flow. Avoid enabling selections until their rendering is wired.

## Engine
- New particle preset IDs in a distinct category, not the existing 0–19 Voxen IDs (saved-scene compatibility).
- Per-instance emitter state: type, position/orientation, scale, seed, density, lifetime, velocity, turbulence, hue, audio reactivity.
- Fixed particle budget, CPU simulation first, one Vulkan instanced billboard pipeline, premultiplied alpha for smoke/snow and additive blending for stars.
- Smoke: drifting translucent expanding sprites; Snow: gravity + lateral drift; Stars: scintillating luminous sprites.
- Bind MP3 FFT bass/mid/treble/RMS to emitter parameters; do not mutate persistent base parameters.
- Scene serialization versioning and graceful unknown-emitter fallback.
- Maintain stereo correctness, Quest 3 frame budget, and existing passthrough Sobel foreground.

## Acceptance criteria
1. PARTICLES tab lists three presets.
2. Controller preview and right-trigger placement work for each.
3. Multiple emitters can coexist, be resized, deleted, saved and restored.
4. Smoke blends properly; snow falls; stars sparkle.
5. FFT optional and no MP3 selected does not affect animation.
6. Vulkan validation clean; stable headset FPS and no regressions in GLB or procedural Voxens.

## Attribution
Open Brush is Apache-2.0 licensed; review asset-specific licenses before copying any texture or code. Prefer original particle sprites and adapted behavior.
