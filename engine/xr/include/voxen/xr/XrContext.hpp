#pragma once

namespace voxen::xr {

// Owns the OpenXR instance/session lifecycle.
// Implementation is added once the pinned OpenXR SDK is wired into Android.
class XrContext final {
public:
    XrContext() = default;
    ~XrContext() = default;

    XrContext(const XrContext&) = delete;
    XrContext& operator=(const XrContext&) = delete;
};

} // namespace voxen::xr
