#pragma once

#include <openxr/openxr.h>

#ifdef __ANDROID__
#include <jni.h>
#endif

namespace voxen::xr {

class XrContext final {
public:
    XrContext() = default;
    ~XrContext();

    XrContext(const XrContext&) = delete;
    XrContext& operator=(const XrContext&) = delete;

#ifdef __ANDROID__
    void initialize(JavaVM* vm, jobject activity);
#else
    void initialize();
#endif
    void shutdown() noexcept;

    [[nodiscard]] XrInstance instance() const noexcept { return instance_; }
    [[nodiscard]] XrSystemId systemId() const noexcept { return systemId_; }
    [[nodiscard]] bool passthroughEnabled() const noexcept { return passthroughEnabled_; }
    [[nodiscard]] bool initialized() const noexcept { return instance_ != XR_NULL_HANDLE; }

private:
#ifdef __ANDROID__
    void initializeLoader(JavaVM* vm, jobject activity);
#endif
    void createInstance();
    void selectSystem();
    void requireExtension(const char* extensionName) const;

    XrInstance instance_ = XR_NULL_HANDLE;
    XrSystemId systemId_ = XR_NULL_SYSTEM_ID;
    bool passthroughEnabled_ = false;
};

} // namespace voxen::xr
