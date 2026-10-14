#pragma once
#include <openxr/openxr.h>
#include <vector>
#include <cstdio>

namespace voxen::xr {
// Native Meta scene discovery. The session owns xrPollEvent and forwards events.
class SceneScanner {
public:
    struct Surface { XrSpace space; XrRect2Df bounds; };
    void initialize(XrInstance instance, XrSession session) {
        session_=session;
        xrGetInstanceProcAddr(instance,"xrQuerySpacesFB",
            reinterpret_cast<PFN_xrVoidFunction*>(&query_));
        xrGetInstanceProcAddr(instance,"xrRetrieveSpaceQueryResultsFB",
            reinterpret_cast<PFN_xrVoidFunction*>(&retrieve_));
        xrGetInstanceProcAddr(instance,"xrGetSpaceBoundingBox2DFB",
            reinterpret_cast<PFN_xrVoidFunction*>(&bounds_));
    }
    bool start() {
        if(!query_ || !retrieve_ || !bounds_ || session_==XR_NULL_HANDLE || request_)return false;
        XrSpaceStorageLocationFilterInfoFB local{
            XR_TYPE_SPACE_STORAGE_LOCATION_FILTER_INFO_FB,nullptr,
            XR_SPACE_STORAGE_LOCATION_LOCAL_FB};
        XrSpaceComponentFilterInfoFB component{
            XR_TYPE_SPACE_COMPONENT_FILTER_INFO_FB,&local,
            XR_SPACE_COMPONENT_TYPE_BOUNDED_2D_FB};
        XrSpaceQueryInfoFB info{
            XR_TYPE_SPACE_QUERY_INFO_FB,nullptr,XR_SPACE_QUERY_ACTION_LOAD_FB,
            128,0,reinterpret_cast<XrSpaceFilterInfoBaseHeaderFB*>(&component),nullptr};
        auto result=query_(session_,
            reinterpret_cast<const XrSpaceQueryInfoBaseHeaderFB*>(&info),&request_);
        if(XR_FAILED(result)){request_=0;return false;}
        return true;
    }
    void onEvent(const XrEventDataBuffer& ev) {
        if(!request_)return;
        if(ev.type==XR_TYPE_EVENT_DATA_SPACE_QUERY_RESULTS_AVAILABLE_FB){
            const auto& e=*reinterpret_cast<const XrEventDataSpaceQueryResultsAvailableFB*>(&ev);
            if(e.requestId!=request_)return;
            XrSpaceQueryResultsFB results{XR_TYPE_SPACE_QUERY_RESULTS_FB};
            if(XR_FAILED(retrieve_(session_,request_,&results)))return;
            std::vector<XrSpaceQueryResultFB> entries(results.resultCountOutput);
            results.resultCapacityInput=static_cast<uint32_t>(entries.size());
            results.results=entries.data();
            if(XR_FAILED(retrieve_(session_,request_,&results)))return;
            for(uint32_t i=0;i<results.resultCountOutput;++i){
                XrRect2Df rect{};
                if(XR_SUCCEEDED(bounds_(session_,entries[i].space,&rect)))
                    surfaces_.push_back({entries[i].space,rect});
            }
        } else if(ev.type==XR_TYPE_EVENT_DATA_SPACE_QUERY_COMPLETE_FB){
            const auto& e=*reinterpret_cast<const XrEventDataSpaceQueryCompleteFB*>(&ev);
            if(e.requestId==request_){
                request_=0;
#ifdef __ANDROID__
                // Keep platform-neutral scanner usable without Android log dependency.
#endif
                std::printf("[Voxen Scene] %zu bounded surfaces discovered\\n",surfaces_.size());
            }
        }
    }
    const std::vector<Surface>& surfaces() const {return surfaces_;}
    void shutdown(){surfaces_.clear();request_=0;session_=XR_NULL_HANDLE;}
private:
    XrSession session_=XR_NULL_HANDLE;
    XrAsyncRequestIdFB request_=0;
    PFN_xrQuerySpacesFB query_=nullptr;
    PFN_xrRetrieveSpaceQueryResultsFB retrieve_=nullptr;
    PFN_xrGetSpaceBoundingBox2DFB bounds_=nullptr;
    std::vector<Surface> surfaces_;
};
} // namespace voxen::xr
