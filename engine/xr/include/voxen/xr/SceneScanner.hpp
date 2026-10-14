#pragma once
#include <openxr/openxr.h>
#include <vector>
#include <cstdio>
#include <string>
#include <algorithm>

namespace voxen::xr {
// Native Meta scene discovery. The session owns xrPollEvent and forwards events.
class SceneScanner {
public:
    struct Surface { XrSpace space; XrRect2Df bounds; std::string labels; bool wall; };
    void initialize(XrInstance instance, XrSession session) {
        session_=session;
        xrGetInstanceProcAddr(instance,"xrQuerySpacesFB",
            reinterpret_cast<PFN_xrVoidFunction*>(&query_));
        xrGetInstanceProcAddr(instance,"xrRetrieveSpaceQueryResultsFB",
            reinterpret_cast<PFN_xrVoidFunction*>(&retrieve_));
        xrGetInstanceProcAddr(instance,"xrGetSpaceSemanticLabelsFB",
            reinterpret_cast<PFN_xrVoidFunction*>(&semantic_));
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
                if(XR_SUCCEEDED(bounds_(session_,entries[i].space,&rect))) {
                    std::string labelText;
                    if(semantic_) {
                        XrSemanticLabelsFB labels{XR_TYPE_SEMANTIC_LABELS_FB};
                        if(XR_SUCCEEDED(semantic_(session_,entries[i].space,&labels))
                           && labels.bufferCountOutput>0 && labels.bufferCountOutput<4096) {
                            std::vector<char> chars(labels.bufferCountOutput+1,0);
                            labels.bufferCapacityInput=labels.bufferCountOutput;
                            labels.buffer=chars.data();
                            if(XR_SUCCEEDED(semantic_(session_,entries[i].space,&labels)))
                                labelText.assign(chars.data(),labels.bufferCountOutput);
                        }
                    }
                    bool wall=false;
                    std::size_t pos=0;
                    while(pos<labelText.size()) {
                        const auto end=labelText.find(',',pos);
                        const auto length=(end==std::string::npos?labelText.size():end)-pos;
                        if(labelText.compare(pos,length,"WALL_FACE")==0)wall=true;
                        if(end==std::string::npos)break;
                        pos=end+1;
                    }
                    surfaces_.push_back({entries[i].space,rect,labelText,wall});
                }
            }
        } else if(ev.type==XR_TYPE_EVENT_DATA_SPACE_QUERY_COMPLETE_FB){
            const auto& e=*reinterpret_cast<const XrEventDataSpaceQueryCompleteFB*>(&ev);
            if(e.requestId==request_){
                request_=0;
#ifdef __ANDROID__
                // Keep platform-neutral scanner usable without Android log dependency.
#endif
                std::printf("[Voxen Scene] %zu bounded surfaces discovered\\n",surfaces_.size());
                for(const auto& surface:surfaces_)if(surface.wall)
                    std::printf("[Voxen Scene] WALL_FACE %.2f x %.2f m\\n",
                        surface.bounds.extent.width,surface.bounds.extent.height);
            }
        }
    }
    const std::vector<Surface>& surfaces() const {return surfaces_;}
    struct WallPose { XrPosef pose; XrRect2Df bounds; };
    std::vector<WallPose> locateWalls(XrSpace baseSpace,XrTime at) const {
        std::vector<WallPose> walls;
        if(baseSpace==XR_NULL_HANDLE)return walls;
        for(const auto& surface:surfaces_) {
            if(!surface.wall)continue;
            XrSpaceLocation location{XR_TYPE_SPACE_LOCATION};
            if(XR_FAILED(xrLocateSpace(surface.space,baseSpace,at,&location)))continue;
            constexpr XrSpaceLocationFlags needed =
                XR_SPACE_LOCATION_POSITION_VALID_BIT | XR_SPACE_LOCATION_ORIENTATION_VALID_BIT;
            if((location.locationFlags & needed)!=needed)continue;
            walls.push_back({location.pose,surface.bounds});
        }
        return walls;
    }
    void shutdown(){surfaces_.clear();request_=0;session_=XR_NULL_HANDLE;}
private:
    XrSession session_=XR_NULL_HANDLE;
    XrAsyncRequestIdFB request_=0;
    PFN_xrQuerySpacesFB query_=nullptr;
    PFN_xrRetrieveSpaceQueryResultsFB retrieve_=nullptr;
    PFN_xrGetSpaceBoundingBox2DFB bounds_=nullptr;
    PFN_xrGetSpaceSemanticLabelsFB semantic_=nullptr;
    std::vector<Surface> surfaces_;
};
} // namespace voxen::xr
