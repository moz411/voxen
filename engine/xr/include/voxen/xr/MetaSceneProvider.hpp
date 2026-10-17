#pragma once
#include <openxr/openxr.h>
#include <voxen/scene/EnvironmentSurface.hpp>
#include <vector>
#include <cstdint>

namespace voxen::xr {
// Querying Meta Scene requires USE_SCENE permission and completed Space Setup.
// All OpenXR calls remain on the XR session thread.
class MetaSceneProvider {
public:
    void initialize(XrInstance instance, XrSession session);
    void request();
    void onEvent(const XrEventDataBaseHeader* event);
    void refresh(XrSpace localSpace, XrTime time);
    void shutdown();
    [[nodiscard]] bool available() const noexcept { return query_ != nullptr; }
    [[nodiscard]] const std::vector<voxen::scene::EnvironmentSurface>& surfaces() const noexcept { return surfaces_; }
private:
    XrSession session_ = XR_NULL_HANDLE;
    XrAsyncRequestIdFB requestId_ = 0;
    bool pending_ = false;
    uint32_t lastWallCount_ = 0;
    PFN_xrQuerySpacesFB query_ = nullptr;
    PFN_xrRetrieveSpaceQueryResultsFB retrieve_ = nullptr;
    PFN_xrGetSpaceBoundingBox2DFB bounds_ = nullptr;
    PFN_xrGetSpaceSemanticLabelsFB labels_ = nullptr;
    std::vector<XrSpaceQueryResultFB> spaces_;
    std::vector<voxen::scene::EnvironmentSurface> surfaces_;
};
} // namespace voxen::xr
