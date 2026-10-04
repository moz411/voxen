#pragma once

#include <jni.h>
#include <openxr/openxr.h>

namespace voxen::xr {

class XrContext final {
public:
    XrContext() = default;
    ~XrContext();

    XrContext(const XrContext&) = delete;
    XrContext& operator=(const XrContext&) = delete;

    void initialize(JavaVM* vm, jobject activity);
    void shutdown() noexcept;

    [[nodiscard]] XrInstance instance() const noexcept { return instance_; }
    [[nodiscard]] XrSystemId systemId() const noexcept { return systemId_; }
    [[nodiscard]] bool initialized() const noexcept { return instance_ != XR_NULL_HANDLE; }

private:
    void initializeLoader(JavaVM* vm, jobject activity);
    void createInstance();
    void selectSystem();
    void requireExtension(const char* extensionName) const;

    XrInstance instance_ = XR_NULL_HANDLE;
    XrSystemId systemId_ = XR_NULL_SYSTEM_ID;
};

} // namespace voxen::xr
