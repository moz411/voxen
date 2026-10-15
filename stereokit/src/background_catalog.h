#pragma once
// Imported source-project index. These are NOT compiled StereoKit shaders.
// Do not bind these entries to unrelated voxel-effect materials.
#include <array>
#include <cstddef>
namespace voxen_backgrounds {
struct Entry { const char* id; const char* name; const char* language; const char* license; };
inline constexpr std::array<Entry,10> entries{{
 {"procedural-noise","PROCEDURAL NOISE","HLSL","CHECK SOURCE"},
 {"drive-home","THE DRIVE HOME","GLSL","CC BY-NC-SA"},
 {"guanabara","GUANABARA","RUST-GPU","CHECK SOURCE"},
 {"seascape","SEASCAPE","RUST-GPU","CC BY-NC-SA"},
 {"iquilez-procedural","IQUILEZ","RUST-GPU","CHECK SOURCE"},
 {"pixelated-cloud","PIXEL CLOUD","GLSL","CHECK SOURCE"},
 {"into-you","INTO YOU","GLSL","CC BY-NC-SA"},
 {"seeded-noise","SEEDED NOISE","GLSL","CHECK SOURCE"},
 {"hlsl-pattern","HLSL PATTERN","HLSL","CHECK SOURCE"},
 {"twirl-sdf","TWIRL SDF","HLSL","CHECK SOURCE"}
}};
} // namespace voxen_backgrounds
