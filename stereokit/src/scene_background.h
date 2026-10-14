#pragma once
// Quest Scene Model planes rendered as transparent shader overlays.
// StereoKit retains exclusive ownership of the OpenXR instance and session.
#include <stereokit.h>
#include <openxr/openxr.h>
#include <algorithm>
#include <array>
#include <cstdio>
#include <cstring>
#include <string>
#include <type_traits>
#include <vector>

namespace voxen_scene {
class Background {
public:
    enum Mask : unsigned { Walls=1, Floor=2, Ceiling=4 };
    int preset = -1; // -1 = no overlay; passthrough remains visible
    unsigned mask = Walls | Floor | Ceiling;
    float opacity = 0.55f;
    bool supported = false;
    bool loading = false;
    bool ready = false;

    void initialize() {
        if (!sk::backend_openxr_ext_enabled(XR_FB_SCENE_EXTENSION_NAME) ||
            !sk::backend_openxr_ext_enabled(XR_FB_SPATIAL_ENTITY_EXTENSION_NAME) ||
            !sk::backend_openxr_ext_enabled(XR_FB_SPATIAL_ENTITY_QUERY_EXTENSION_NAME)) {
            std::printf("[Voxen Scene] Scene extensions unavailable\n");
            return;
        }
        session_ = reinterpret_cast<XrSession>(sk::backend_openxr_get_session());
        auto load=[&](const char* name, auto& target) {
            using Proc=std::decay_t<decltype(target)>;
            target=reinterpret_cast<Proc>(sk::backend_openxr_get_function(name));
            return target!=nullptr;
        };
        supported =
            load("xrQuerySpacesFB",query_) &&
            load("xrRetrieveSpaceQueryResultsFB",results_) &&
            load("xrGetSpaceBoundingBox2DFB",bounds_) &&
            load("xrGetSpaceSemanticLabelsFB",labels_) &&
            load("xrSetSpaceComponentStatusFB",set_status_);
        if (!supported) return;
        sk::backend_openxr_add_callback_poll_event(&Background::event_callback, this);
        refresh();
    }

    void refresh() {
        if (!supported || loading) return;
        clear_planes();
        ready = false;
        XrSpaceComponentFilterInfoFB filter{XR_TYPE_SPACE_COMPONENT_FILTER_INFO_FB};
        filter.componentType = XR_SPACE_COMPONENT_TYPE_BOUNDED_2D_FB;
        XrSpaceQueryInfoFB query{XR_TYPE_SPACE_QUERY_INFO_FB};
        query.queryAction=XR_SPACE_QUERY_ACTION_LOAD_FB;
        query.maxResultCount=128;
        query.filter=reinterpret_cast<const XrSpaceFilterInfoBaseHeaderFB*>(&filter);
        const XrResult result=query_(session_,
            reinterpret_cast<const XrSpaceQueryInfoBaseHeaderFB*>(&query), &request_);
        loading = XR_SUCCEEDED(result);
        if (!loading) std::printf("[Voxen Scene] xrQuerySpacesFB failed: %d\n", result);
    }

    void render(sk::mesh_t mesh, const std::array<sk::material_t, 32>& materials) {
        if (preset < 0 || !ready || !mesh || preset >= static_cast<int>(materials.size())) return;
        sk::material_t mat=materials[preset];
        if (!mat) return;
        const XrSpace base=reinterpret_cast<XrSpace>(sk::backend_openxr_get_space());
        const XrTime time=static_cast<XrTime>(sk::backend_openxr_get_time());
        if (base==XR_NULL_HANDLE || time==0) return;
        // Scene planes live in the same tracking reference space as the rest of StereoKit.
        for (const Plane& p : planes_) {
            if (!(p.category & mask)) continue;
            XrSpaceLocation location{XR_TYPE_SPACE_LOCATION};
            if (XR_FAILED(xrLocateSpace(p.space, base, time, &location))) continue;
            constexpr XrSpaceLocationFlags required =
                XR_SPACE_LOCATION_POSITION_VALID_BIT | XR_SPACE_LOCATION_ORIENTATION_VALID_BIT;
            if ((location.locationFlags & required) != required) continue;
            const auto& q=location.pose.orientation;
            sk::quat orientation{q.x,q.y,q.z,q.w};
            const auto& t=location.pose.position;
            // Bounding box offset is expressed in the anchor's local XY plane.
            const sk::vec3 offset=sk::quat_mul_vec(orientation,
                {p.rect.offset.x+p.rect.extent.width*0.5f,
                 p.rect.offset.y+p.rect.extent.height*0.5f,0});
            sk::vec3 position{t.x+offset.x,t.y+offset.y,t.z+offset.z};
            const float w=p.rect.extent.width,h=p.rect.extent.height;
            if (w<=0 || h<=0) continue;
            sk::render_add_mesh(mesh,mat,
                sk::matrix_trs(position,orientation,{w,h,0.002f}),
                {1,1,1,opacity});
        }
    }

    int plane_count() const { return static_cast<int>(planes_.size()); }

    void shutdown() {
        // StereoKit exposes registration only. Callback remains installed until sk_shutdown;
        // this object has static lifetime and no subsequent OpenXR event polling occurs.
        loading=false;
        ready=false;
        clear_planes();
        supported=false;
    }

private:
    struct Plane { XrSpace space; XrRect2Df rect; unsigned category; };
    std::vector<Plane> planes_;
    XrSession session_=XR_NULL_HANDLE;
    XrAsyncRequestIdFB request_=0;
    PFN_xrQuerySpacesFB query_=nullptr;
    PFN_xrRetrieveSpaceQueryResultsFB results_=nullptr;
    PFN_xrGetSpaceBoundingBox2DFB bounds_=nullptr;
    PFN_xrGetSpaceSemanticLabelsFB labels_=nullptr;
    PFN_xrSetSpaceComponentStatusFB set_status_=nullptr;

    void clear_planes() {
        for (const auto& p : planes_) if (p.space!=XR_NULL_HANDLE) xrDestroySpace(p.space);
        planes_.clear();
    }

    static void event_callback(void* context, void* event) {
        auto* self=static_cast<Background*>(context);
        const auto* header=static_cast<const XrEventDataBaseHeader*>(event);
        if (!self || !header || !self->loading) return;
        if (header->type==XR_TYPE_EVENT_DATA_SPACE_QUERY_RESULTS_AVAILABLE_FB) {
            const auto* e=reinterpret_cast<const XrEventDataSpaceQueryResultsAvailableFB*>(header);
            if (e->requestId==self->request_) self->read_results();
        } else if (header->type==XR_TYPE_EVENT_DATA_SPACE_QUERY_COMPLETE_FB) {
            const auto* e=reinterpret_cast<const XrEventDataSpaceQueryCompleteFB*>(header);
            if (e->requestId!=self->request_) return;
            self->loading=false;
            self->ready=XR_SUCCEEDED(e->result);
            std::printf("[Voxen Scene] query complete: %zu planes, result=%d\n",
                self->planes_.size(),e->result);
        }
    }

    void read_results() {
        XrSpaceQueryResultsFB out{XR_TYPE_SPACE_QUERY_RESULTS_FB};
        if (XR_FAILED(results_(session_,request_,&out)) || !out.resultCountOutput) return;
        std::vector<XrSpaceQueryResultFB> results(out.resultCountOutput);
        out.resultCapacityInput=static_cast<uint32_t>(results.size());
        out.results=results.data();
        if (XR_FAILED(results_(session_,request_,&out))) return;
        for (uint32_t i=0; i<out.resultCountOutput; ++i) {
            const XrSpace space=results[i].space;
            XrRect2Df rect{};
            if (XR_FAILED(bounds_(session_,space,&rect))) { xrDestroySpace(space); continue; }
            XrSemanticLabelsFB semantic{XR_TYPE_SEMANTIC_LABELS_FB};
            if (XR_FAILED(labels_(session_,space,&semantic)) || !semantic.bufferCountOutput) {
                xrDestroySpace(space); continue;
            }
            std::vector<char> label_buffer(semantic.bufferCountOutput+1,0);
            semantic.bufferCapacityInput=static_cast<uint32_t>(label_buffer.size());
            semantic.buffer=label_buffer.data();
            if (XR_FAILED(labels_(session_,space,&semantic))) { xrDestroySpace(space); continue; }
            const std::string names(label_buffer.data());
            unsigned category=0;
            if (names.find("WALL_FACE")!=std::string::npos &&
                names.find("INVISIBLE_WALL_FACE")==std::string::npos) category=Walls;
            else if (names.find("FLOOR")!=std::string::npos) category=Floor;
            else if (names.find("CEILING")!=std::string::npos) category=Ceiling;
            if (!category) { xrDestroySpace(space); continue; }
            XrSpaceComponentStatusSetInfoFB locatable{XR_TYPE_SPACE_COMPONENT_STATUS_SET_INFO_FB};
            locatable.componentType=XR_SPACE_COMPONENT_TYPE_LOCATABLE_FB;
            locatable.enabled=XR_TRUE;
            XrAsyncRequestIdFB locate_request=0;
            const XrResult locate_result=set_status_(space,&locatable,&locate_request);
            if (XR_FAILED(locate_result)) {
                std::printf("[Voxen Scene] locatable unavailable: %d\\n",locate_result);
            }
            planes_.push_back({space,rect,category});
        }
    }
};
} // namespace voxen_scene
