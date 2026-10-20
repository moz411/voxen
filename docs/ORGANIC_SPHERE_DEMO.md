# Organic sphere — shader prototype

This experimental branch adds a vertex-displaced, fragment-shaded 3D sphere.

- Procedural sphere topology: 48 longitude segments × 32 latitude rings, 3072 triangles.
- Animated multi-frequency trigonometric displacement and breathing.
- Finite-difference normals, diffuse lighting, iridescent palette and rim light.
- Existing Vulkan push constant layout: mat4 mvp + vec4 parameters (time, speed, hue, amplitude).
- No textures, descriptor sets, geometry buffers or compute passes required.

**Integration still required:** add shader compilation to engine/CMakeLists.txt, build a graphics pipeline in XrSwapchainRenderer, add a demo preset and draw 9216 vertices with vkCmdDraw. Existing draw path currently uses six vertices for plasma; simply swapping shader files will not display the sphere.

For a first test use amplitude 0.26, speed 1.0, hue 0.56. Retain Sobel passthrough and do not modify main.
