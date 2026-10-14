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
#include <vector>

namespace voxen_scene {
class SceneManager {
    PFN_xrQuerySpacesFB query_ = nullptr;
    PFN_xrRetrieveSpaceQueryResultsFB retrieve_ = nullptr;
    PFN_xrGetSpaceBoundingBox2DFB bounds_ = nullptr;
    XrAsyncRequestIdFB pending_ = 0;
    bool enabled_ = false;
    XrSession session_ = XR_NULL_HANDLE;
    std::vector<XrSpace> planes_;
    static void on_event(void* ctx, void* ev) {
        auto& self=*static_cast<SceneManager*>(ctx);
        self.handle_event(*static_cast<const XrEventDataBuffer*>(ev));
    }
    void handle_event(const XrEventDataBuffer& ev) {
        if (!pending_) return;
        if (ev.type == XR_TYPE_EVENT_DATA_SPACE_QUERY_RESULTS_AVAILABLE_FB) {
            const auto& event=*reinterpret_cast<const XrEventDataSpaceQueryResultsAvailableFB*>(&ev);
            if(event.requestId != pending_) return;
            XrSpaceQueryResultsFB results{XR_TYPE_SPACE_QUERY_RESULTS_FB};
            if(XR_FAILED(retrieve_(session_,pending_,&results))) return;
            std::vector<XrSpaceQueryResultFB> entries(results.resultCountOutput);
            results.resultCapacityInput=static_cast<uint32_t>(entries.size());
            results.results=entries.data();
            if(XR_FAILED(retrieve_(session_,pending_,&results))) return;
            for(uint32_t i=0;i<results.resultCountOutput;++i) {
                XrRect2Df rect{};
                if(XR_SUCCEEDED(bounds_(session_,entries[i].space,&rect))) {
                    planes_.push_back(entries[i].space);
                    std::printf("[Voxen Scene] plane %zu: %.2f x %.2f m\\n",
                        planes_.size(),rect.extent.width,rect.extent.height);
                }
            }
        } else if(ev.type == XR_TYPE_EVENT_DATA_SPACE_QUERY_COMPLETE_FB) {
            const auto& event=*reinterpret_cast<const XrEventDataSpaceQueryCompleteFB*>(&ev);
            if(event.requestId == pending_) {
                pending_=0;
                std::printf("[Voxen Scene] query completed; %zu planes\\n",planes_.size());
            }
        }
    }
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
        if(enabled_) {
            session_=reinterpret_cast<XrSession>(sk::backend_openxr_get_session());
            sk::backend_openxr_add_callback_poll_event(on_event,this);
            query_bounded_planes(session_);
        }
    }
    bool available() const { return enabled_; }
    std::size_t plane_count() const { return planes_.size(); }
    void shutdown() {
        if(enabled_) sk::backend_openxr_remove_callback_poll_event(on_event);
        enabled_=false;
        pending_=0;
        planes_.clear();
    }

    // Query saved bounded planes; query results are received by StereoKit event callback.
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
