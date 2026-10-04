# Voxen

A lightweight OpenXR/Vulkan voxel engine for spatial computing.

## First milestone

The initial milestone targets Meta Quest 3 with a deliberately small native stack:

- C++20
- Android NDK + CMake
- OpenXR
- Vulkan
- native Android activity

The first executable goal is an OpenXR session using Vulkan on Quest. Voxel rendering, passthrough and editor UI will be layered on top once the XR/Vulkan lifecycle is stable.

## Layout

```text
android/          Android application shell
engine/
  core/           engine lifecycle
  xr/             OpenXR integration
  render/         Vulkan renderer
  voxel/          voxel world/materials (next milestone)
```

## Prerequisites

- Android Studio / Android SDK
- Android NDK 27+
- CMake 3.22+
- OpenXR headers/loader available to the native build

The build integration for the OpenXR Android loader is intentionally kept explicit; the next bootstrap step will vendor or fetch a pinned OpenXR SDK version and produce the first installable Quest APK.
