#pragma once
// Meta Scene API integration point for Voxen's StereoKit backend.
// The scene query is asynchronous: StereoKit owns OpenXR event polling.
// Results must be handled through an event callback (not a second xrPollEvent).
#include <stereokit.h>
#include <openxr/openxr.h>
#include <openxr/fb_spatial_entity.h>
#include <openxr/fb_spatial_entity_query.h>
#include <openxr/fb_spatial_entity_storage.h>
#include <openxr/fb_scene.h>
#include <cstdio>

namespace voxen_scene {
class SceneManager {
    PFN_xrQuerySpacesFB query_ = nullptr;
    PFN_xrRetrieveSpaceQueryResultsFB retrieve_ = nullptr;
    PFN_xrGetSpaceBoundingBox2DFB bounds_ = nullptr;
    XrAsyncRequestIdFB pending_ = 0;
    bool enabled_ = false;
public:
    void initialize() {
        using namespace sk;
        enabled_ =
            backend_xr_get_type() == backend_xr_type_openxr &&
            backend_openxr_ext_enabled(XR_FB_SPATIAL_ENTITY_EXTENSION_NAME) &&
            backend_openxr_ext_enabled(XR_FB_SPATIAL_ENTITY_QUERY_EXTENSION_NAME) &&
            backend_openxr_ext_enabled(XR_FB_SCENE_EXTENSION_NAME);
        if (!enabled_) {
            std::printf("[Voxen Scene] Required Meta Scene extensions unavailable; disabled.\\n");
            return;
        }
        query_ = reinterpret_cast<PFN_xrQuerySpacesFB>(backend_openxr_get_function("xrQuerySpacesFB"));
        retrieve_ = reinterpret_cast<PFN_xrRetrieveSpaceQueryResultsFB>(
            backend_openxr_get_function("xrRetrieveSpaceQueryResultsFB"));
        bounds_ = reinterpret_cast<PFN_xrGetSpaceBoundingBox2DFB>(
            backend_openxr_get_function("xrGetSpaceBoundingBox2DFB"));
        enabled_ = query_ && retrieve_ && bounds_;
        std::printf("[Voxen Scene] API %s\\n", enabled_ ? "available" : "function pointers missing");
    }
    bool available() const { return enabled_; }

    // Starts reading locally saved scene planes. No new scene capture is forced.
    // Call only once an event handler for XR_FB query events has been installed.
    bool query_bounded_planes(XrSession session) {
        if (!enabled_ || session == XR_NULL_HANDLE || pending_) return false;
        XrSpaceStorageLocationFilterInfoFB local{
            XR_TYPE_SPACE_STORAGE_LOCATION_FILTER_INFO_FB, nullptr,
            XR_SPACE_STORAGE_LOCATION_LOCAL_FB};
        XrSpaceComponentFilterInfoFB bounded{
            XR_TYPE_SPACE_COMPONENT_FILTER_INFO_FB, &local,
            XR_SPACE_COMPONENT_TYPE_BOUNDED_2D_FB};
        XrSpaceQueryInfoFB info{
            XR_TYPE_SPACE_QUERY_INFO_FB, nullptr, XR_SPACE_QUERY_ACTION_LOAD_FB,
            128, 0, reinterpret_cast<XrSpaceFilterInfoBaseHeaderFB*>(&bounded), nullptr};
        const XrResult rc = query_(
            session,reinterpret_cast<const XrSpaceQueryInfoBaseHeaderFB*>(&info),&pending_);
        if (XR_FAILED(rc)) {
            pending_ = 0;
            std::printf("[Voxen Scene] xrQuerySpacesFB error: %d\\n", int(rc));
            return false;
        }
        return true;
    }
    // Hook into StereoKit's OpenXR event callback before enabling query_bounded_planes.
    // Query results require xrRetrieveSpaceQueryResultsFB; get semantic WALL_FACE labels,
    // BOUNDED_2D rectangle and LOCATABLE pose before creating the wall quad.
    XrAsyncRequestIdFB pending_request() const { return pending_; }
};
} // namespace voxen_scene
