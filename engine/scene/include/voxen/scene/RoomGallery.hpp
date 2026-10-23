#pragma once
#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace voxen::gallery {

// These are stable *application* IDs, never transient OpenXR handles.
using SceneId = std::string;
using RoomId = std::string;
using InstanceId = std::string;

struct Pose {
    std::array<float, 3> positionMeters{};
    std::array<float, 4> orientationXyzw{0.f, 0.f, 0.f, 1.f};
};

struct Preview {
    // Lightweight, serialized proxy; no full Vulkan buffers or decoded MP3 in gallery.
    std::string proxyAsset;
    float boundingRadiusMeters = 0.25f;
    std::uint32_t materialPreset = 0;
};

struct SceneRecord {
    SceneId id;
    std::string displayName;
    std::uint32_t schemaVersion = 1;
    std::string payloadAsset; // Versioned scene document, relative to app-private storage.
    Preview preview;
};

struct Placement {
    InstanceId id;
    SceneId sceneId;
    Pose roomLocalPose; // Valid only after the corresponding persistent room anchor is localized.
    float scale = 1.f;
};

struct RoomGallery {
    RoomId roomId;
    std::string persistentAnchorId; // App-managed durable reference; not an XrSpace handle.
    std::vector<Placement> placements;
};

enum class Mode : std::uint8_t {
    Gallery, Entering, Immersive, Leaving, LocalizationUnavailable, Error
};

// Orchestrates ownership transitions; the renderer/audio/anchor backends perform the actual work.
// Calls are intentionally synchronous state transitions, not fake resource-freeing operations.
class Lifecycle {
public:
    [[nodiscard]] Mode mode() const noexcept { return mode_; }
    [[nodiscard]] const std::optional<SceneId>& activeScene() const noexcept { return active_; }

    bool requestEnter(const SceneId& id) {
        if (mode_ != Mode::Gallery || id.empty()) return false;
        pending_ = id;
        mode_ = Mode::Entering;
        return true;
    }

    // Call only AFTER GPU fences have completed, gallery-only resources were destroyed,
    // and the selected scene's assets and simulation have been loaded successfully.
    bool completeEnter() {
        if (mode_ != Mode::Entering || !pending_) return false;
        active_ = std::move(pending_);
        pending_.reset();
        mode_ = Mode::Immersive;
        return true;
    }

    bool requestLeave() {
        if (mode_ != Mode::Immersive) return false;
        mode_ = Mode::Leaving;
        return true;
    }

    // Call only AFTER immersive audio/simulation stop and their owned CPU/GPU assets
    // have been destroyed; gallery previews may then be re-created from saved proxies.
    bool completeLeave() {
        if (mode_ != Mode::Leaving) return false;
        active_.reset();
        mode_ = Mode::Gallery;
        return true;
    }

    void localizationLost() {
        // Do not draw previously saved placements at arbitrary LOCAL coordinates.
        if (mode_ == Mode::Gallery) mode_ = Mode::LocalizationUnavailable;
    }

    void localizationRestored() {
        if (mode_ == Mode::LocalizationUnavailable) mode_ = Mode::Gallery;
    }

    void fail() { mode_ = Mode::Error; }
    void resetToGallery() {
        pending_.reset();
        active_.reset();
        mode_ = Mode::Gallery;
    }

private:
    Mode mode_ = Mode::Gallery;
    std::optional<SceneId> pending_;
    std::optional<SceneId> active_;
};

} // namespace voxen::gallery
