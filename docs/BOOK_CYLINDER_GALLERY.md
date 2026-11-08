# Book-cover cylinder gallery — first increment

Branch: `feature/exhibition-book-cylinders`

## Scope
A cylindrical gallery portal displays an original, independently licensed cover
image. The cylinder is **presentation only**: keep the current walk-in/out
state machine, passthrough Sobel, gallery persistence and localization checks.

## Geometry
`BookCylinder.hpp` generates a shared 48-segment unit side mesh, with
outward normals, seam vertices and three repetitions of the cover texture.
Scale by 0.4 m radius and 1.6 m height by default. Use REPEAT sampler
addressing, mipmaps, and a two-sided material only if interior viewing is
desired. Avoid rotating the geometry or camera automatically.

## Assets
Read covers from the existing Android app-private media storage, in a new
`Books/` subfolder. Prefer JPEG or PNG with 1:2-ish portrait ratio;
1024x2048 is an upper starting budget, not a mandatory texture size.
Asset references must be relative paths; reject traversal and missing files.

## Next integration steps
1. Extend `SceneRecord` or placement presentation metadata with a versioned
   optional `BookCylinderStyle` reference; default old records to the
   existing sphere geometry. Update SceneRepository serialization/migration.
2. Reuse the existing Vulkan texture loader and upload path for the cover.
   Share one mesh and pipeline among cylinders; do not allocate a pipeline
   per portal. Keep Sobel compositing unchanged.
3. Extend gallery hover, grab, delete and selection tests to the cylinder
   dimensions; do not replace the existing walk-in/out behavior without
   explicit validation. Ensure that collision and visual volume agree.
4. Handle missing covers with a neutral procedural fallback.
5. Test repeated enter/leave, tracking loss, app restart, GPU memory and
   frame time on Quest 3.

## Rights
The visual assets should be original designs. Denoël book-cover artwork
must not be bundled unless permission is obtained. Titles and bibliographic
metadata can be maintained separately from the cover imagery.
