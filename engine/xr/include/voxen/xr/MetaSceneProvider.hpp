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
    struct CapturedMesh {
        XrSpace space = XR_NULL_HANDLE;
        XrPosef pose{{0.f,0.f,0.f,1.f},{0.f,0.f,0.f}};
        bool located = false;
        std::vector<XrVector3f> vertices;
        std::vector<uint32_t> indices;
    };
    [[nodiscard]] const std::vector<CapturedMesh>& meshes() const noexcept { return capturedMeshes_; }
    [[nodiscard]] bool available() const noexcept { return query_ != nullptr; }
    [[nodiscard]] const std::vector<voxen::scene::EnvironmentSurface>& surfaces() const noexcept { return surfaces_; }
private:
    XrSession session_ = XR_NULL_HANDLE;
    XrAsyncRequestIdFB requestId_ = 0;
    bool pending_ = false;
    uint32_t lastWallCount_ = 0;
    PFN_xrQuerySpacesFB query_ = nullptr;
    PFN_xrDiscoverSpacesMETA discover_ = nullptr;
    PFN_xrRetrieveSpaceDiscoveryResultsMETA retrieveDiscovery_ = nullptr;
    bool usingDiscovery_ = false;
    PFN_xrGetSpaceComponentStatusFB getStatus_ = nullptr;
    PFN_xrSetSpaceComponentStatusFB setStatus_ = nullptr;
    std::vector<XrSpace> requestedLabels_;
    std::vector<XrSpace> requestedBounds_;
    std::vector<XrSpace> requestedLocatable_;
    struct UnsupportedComponent { XrSpace space; XrSpaceComponentTypeFB type; };
    std::vector<UnsupportedComponent> unsupportedComponents_;
    struct CachedPlane { XrSpace space; XrRect2Df rect; bool wall; };
    std::vector<CachedPlane> cachedPlanes_;
    std::vector<XrSpace> requestedMesh_;
    std::vector<XrSpace> inspectedMesh_;
    std::vector<CapturedMesh> capturedMeshes_;
    PFN_xrGetSpaceTriangleMeshMETA getTriangleMesh_ = nullptr;
    PFN_xrRetrieveSpaceQueryResultsFB retrieve_ = nullptr;
    PFN_xrGetSpaceBoundingBox2DFB bounds_ = nullptr;
    PFN_xrGetSpaceSemanticLabelsFB labels_ = nullptr;
    std::vector<XrSpaceQueryResultFB> spaces_;
    std::vector<voxen::scene::EnvironmentSurface> surfaces_;
};
} // namespace voxen::xr
