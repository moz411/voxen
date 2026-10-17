#include <voxen/xr/MetaSceneProvider.hpp>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <string>
#include <vector>
#ifdef __ANDROID__
#include <android/log.h>
#else
#include <cstdio>
#endif

namespace voxen::xr {
namespace {
void sceneLog(const char* message, int value) {
#ifdef __ANDROID__
    __android_log_print(ANDROID_LOG_INFO,"VoxenScene","%s: %d",message,value);
#else
    std::printf("[VoxenScene] %s: %d\n",message,value);
#endif
}
XrVector3f rotate(XrQuaternionf q,XrVector3f v) {
    const XrVector3f u{q.x,q.y,q.z};
    const XrVector3f t{2.f*(u.y*v.z-u.z*v.y),
                         2.f*(u.z*v.x-u.x*v.z),
                         2.f*(u.x*v.y-u.y*v.x)};
    return {v.x+q.w*t.x+u.y*t.z-u.z*t.y,
            v.y+q.w*t.y+u.z*t.x-u.x*t.z,
            v.z+q.w*t.z+u.x*t.y-u.y*t.x};
}
}
void MetaSceneProvider::initialize(XrInstance instance,XrSession session) {
    session_=session;
    auto load=[&](const char* name,PFN_xrVoidFunction* function) {
        return XR_SUCCEEDED(xrGetInstanceProcAddr(instance,name,function)) && *function;
    };
    if(!load("xrQuerySpacesFB",reinterpret_cast<PFN_xrVoidFunction*>(&query_)) ||
       !load("xrRetrieveSpaceQueryResultsFB",reinterpret_cast<PFN_xrVoidFunction*>(&retrieve_)) ||
       !load("xrGetSpaceBoundingBox2DFB",reinterpret_cast<PFN_xrVoidFunction*>(&bounds_)) ||
       !load("xrGetSpaceSemanticLabelsFB",reinterpret_cast<PFN_xrVoidFunction*>(&labels_))) {
        query_=nullptr;
        sceneLog("Scene query functions unavailable",0);
    }
    load("xrGetSpaceComponentStatusFB",reinterpret_cast<PFN_xrVoidFunction*>(&getStatus_));
    load("xrSetSpaceComponentStatusFB",reinterpret_cast<PFN_xrVoidFunction*>(&setStatus_));
    if(load("xrDiscoverSpacesMETA",reinterpret_cast<PFN_xrVoidFunction*>(&discover_)) &&
       load("xrRetrieveSpaceDiscoveryResultsMETA",
            reinterpret_cast<PFN_xrVoidFunction*>(&retrieveDiscovery_))) {
        usingDiscovery_=true;
        sceneLog("META discovery backend enabled",1);
    } else {
        discover_=nullptr;
        retrieveDiscovery_=nullptr;
        usingDiscovery_=false;
        sceneLog("META discovery backend unavailable",0);
    }
}
void MetaSceneProvider::request() {
    if(pending_)return;
    if(usingDiscovery_) {
        // No filters on first pass: enumerate accessible persistent spatial
        // entities. Scene semantic / bounded-2D filtering is done afterward.
        XrSpaceDiscoveryInfoMETA info{XR_TYPE_SPACE_DISCOVERY_INFO_META};
        const XrResult result=discover_(session_,&info,&requestId_);
        pending_=XR_SUCCEEDED(result);
        sceneLog("META discovery request result",static_cast<int>(result));
        return;
    }
    if(!query_)return;
    // Meta's Scene API expects the local-storage filter chained to the
    // component filter when querying room-capture plane anchors.
    // This runtime rejects a storage-only filter (XR_ERROR_VALIDATION_FAILURE).
    // Query BOUNDED_2D anchors in LOCAL storage; an empty successful query means
    // no matching captured wall planes are currently accessible.
    XrSpaceStorageLocationFilterInfoFB storage{
        XR_TYPE_SPACE_STORAGE_LOCATION_FILTER_INFO_FB};
    storage.location=XR_SPACE_STORAGE_LOCATION_LOCAL_FB;
    XrSpaceComponentFilterInfoFB filter{XR_TYPE_SPACE_COMPONENT_FILTER_INFO_FB};
    filter.next=&storage;
    filter.componentType=XR_SPACE_COMPONENT_TYPE_BOUNDED_2D_FB;
    XrSpaceQueryInfoFB info{XR_TYPE_SPACE_QUERY_INFO_FB};
    info.queryAction=XR_SPACE_QUERY_ACTION_LOAD_FB;
    info.maxResultCount=256;
    info.timeout=0;
    info.filter=reinterpret_cast<const XrSpaceFilterInfoBaseHeaderFB*>(&filter);
    auto result=query_(session_,reinterpret_cast<const XrSpaceQueryInfoBaseHeaderFB*>(&info),&requestId_);
    pending_=XR_SUCCEEDED(result);
    sceneLog("Scene bounded-2D local query result",static_cast<int>(result));
}
void MetaSceneProvider::onEvent(const XrEventDataBaseHeader* event) {
    if(!event)return;
    if(event->type==XR_TYPE_EVENT_DATA_SPACE_SET_STATUS_COMPLETE_FB) {
        const auto* e=reinterpret_cast<const XrEventDataSpaceSetStatusCompleteFB*>(event);
        sceneLog("Space component enable complete",static_cast<int>(e->result));
        return;
    }
    if(!pending_)return;
    if(usingDiscovery_ && event->type==XR_TYPE_EVENT_DATA_SPACE_DISCOVERY_RESULTS_AVAILABLE_META) {
        const auto* e=reinterpret_cast<const XrEventDataSpaceDiscoveryResultsAvailableMETA*>(event);
        if(e->requestId!=requestId_)return;
        XrSpaceDiscoveryResultsMETA results{XR_TYPE_SPACE_DISCOVERY_RESULTS_META};
        const XrResult countResult=retrieveDiscovery_(session_,requestId_,&results);
        sceneLog("META discovery retrieve count result",static_cast<int>(countResult));
        sceneLog("META discovery candidate count",static_cast<int>(results.resultCountOutput));
        if((XR_FAILED(countResult) && countResult!=XR_ERROR_SIZE_INSUFFICIENT) ||
           results.resultCountOutput==0)return;
        std::vector<XrSpaceDiscoveryResultMETA> batch(results.resultCountOutput);
        results.resultCapacityInput=static_cast<uint32_t>(batch.size());
        results.results=batch.data();
        const XrResult dataResult=retrieveDiscovery_(session_,requestId_,&results);
        sceneLog("META discovery retrieve data result",static_cast<int>(dataResult));
        if(XR_SUCCEEDED(dataResult)) {
            for(uint32_t i=0;i<results.resultCountOutput;i++) {
                XrSpaceQueryResultFB space{};
                space.space=batch[i].space;
                space.uuid=batch[i].uuid;
                const bool known=std::any_of(spaces_.begin(),spaces_.end(),
                    [&](const auto& existing) {
                        return std::memcmp(existing.uuid.data,space.uuid.data,
                                           sizeof(space.uuid.data))==0;
                    });
                if(!known)spaces_.push_back(space);
            }
            sceneLog("META discovery spaces retrieved",static_cast<int>(spaces_.size()));
        }
    } else if(usingDiscovery_ && event->type==XR_TYPE_EVENT_DATA_SPACE_DISCOVERY_COMPLETE_META) {
        const auto* e=reinterpret_cast<const XrEventDataSpaceDiscoveryCompleteMETA*>(event);
        if(e->requestId==requestId_) {
            // Completion itself can signal that results are ready.
            if(XR_SUCCEEDED(e->result) && spaces_.empty()) {
                XrSpaceDiscoveryResultsMETA results{XR_TYPE_SPACE_DISCOVERY_RESULTS_META};
                const XrResult countResult=retrieveDiscovery_(session_,requestId_,&results);
                sceneLog("META complete retrieve count result",static_cast<int>(countResult));
                sceneLog("META complete candidate count",static_cast<int>(results.resultCountOutput));
                if((XR_SUCCEEDED(countResult) || countResult==XR_ERROR_SIZE_INSUFFICIENT)
                    && results.resultCountOutput>0) {
                    std::vector<XrSpaceDiscoveryResultMETA> batch(results.resultCountOutput);
                    results.resultCapacityInput=static_cast<uint32_t>(batch.size());
                    results.results=batch.data();
                    const XrResult dataResult=retrieveDiscovery_(session_,requestId_,&results);
                    sceneLog("META complete retrieve data result",static_cast<int>(dataResult));
                    if(XR_SUCCEEDED(dataResult)) {
                        for(uint32_t i=0;i<results.resultCountOutput;i++) {
                            XrSpaceQueryResultFB space{};
                            space.space=batch[i].space;
                            space.uuid=batch[i].uuid;
                            const bool known=std::any_of(spaces_.begin(),spaces_.end(),
                                [&](const auto& existing) {
                                    return std::memcmp(existing.uuid.data,space.uuid.data,
                                        sizeof(space.uuid.data))==0;
                                });
                            if(!known)spaces_.push_back(space);
                        }
                    }
                }
            }
            pending_=false;
            sceneLog("META discovery complete",static_cast<int>(e->result));
            sceneLog("META discovery total spaces",static_cast<int>(spaces_.size()));
        }
    } else if(!usingDiscovery_ && event->type==XR_TYPE_EVENT_DATA_SPACE_QUERY_RESULTS_AVAILABLE_FB) {
        const auto* e=reinterpret_cast<const XrEventDataSpaceQueryResultsAvailableFB*>(event);
        if(e->requestId!=requestId_)return;
        XrSpaceQueryResultsFB results{XR_TYPE_SPACE_QUERY_RESULTS_FB};
        const XrResult countResult=retrieve_(session_,requestId_,&results);
        sceneLog("Scene retrieve count result",static_cast<int>(countResult));
        sceneLog("Scene retrieve candidate count",static_cast<int>(results.resultCountOutput));
        // Some runtimes return XR_ERROR_SIZE_INSUFFICIENT while reporting
        // the required count: do not mistake that for an empty scene.
        if((XR_FAILED(countResult) && countResult!=XR_ERROR_SIZE_INSUFFICIENT) ||
           results.resultCountOutput==0)return;
        std::vector<XrSpaceQueryResultFB> batch(results.resultCountOutput);
        results.resultCapacityInput=static_cast<uint32_t>(batch.size());
        results.results=batch.data();
        const XrResult batchResult=retrieve_(session_,requestId_,&results);
        sceneLog("Scene retrieve data result",static_cast<int>(batchResult));
        if(XR_SUCCEEDED(batchResult)) {
            for(uint32_t i=0;i<results.resultCountOutput;i++)spaces_.push_back(batch[i]);
            sceneLog("Scene spaces retrieved",static_cast<int>(spaces_.size()));
        }
    } else if(!usingDiscovery_ && event->type==XR_TYPE_EVENT_DATA_SPACE_QUERY_COMPLETE_FB) {
        const auto* e=reinterpret_cast<const XrEventDataSpaceQueryCompleteFB*>(event);
        if(e->requestId==requestId_) {
            pending_=false;
            sceneLog("Scene query complete",static_cast<int>(e->result));
            sceneLog("Scene total anchor results",static_cast<int>(spaces_.size()));
        }
    }
}
void MetaSceneProvider::refresh(XrSpace localSpace,XrTime time) {
    if(!bounds_ || !labels_)return;
    std::vector<voxen::scene::EnvironmentSurface> walls;
    for(const auto& result:spaces_) {
        // Scene anchors are discovered before their components are enabled.
        // Schedule at most one asynchronous enable per space/component and
        // poll component status on later frames. Do not block XR frame loop.
        auto enabled=[&](XrSpaceComponentTypeFB type,std::vector<XrSpace>& requested) {
            if(!getStatus_ || !setStatus_)return false;
            XrSpaceComponentStatusFB status{XR_TYPE_SPACE_COMPONENT_STATUS_FB};
            const XrResult statusResult=getStatus_(result.space,type,&status);
            if(XR_FAILED(statusResult))return false;
            if(status.enabled==XR_TRUE)return true;
            if(status.changePending==XR_TRUE ||
                std::find(requested.begin(),requested.end(),result.space)!=requested.end())
                return false;
            XrSpaceComponentStatusSetInfoFB info{XR_TYPE_SPACE_COMPONENT_STATUS_SET_INFO_FB};
            info.componentType=type;
            info.enabled=XR_TRUE;
            info.timeout=0;
            XrAsyncRequestIdFB id=0;
            const XrResult requestResult=setStatus_(result.space,&info,&id);
            // A failed request must not be retried every XR frame: some
            // anchors do not support all optional components. Keep probing
            // status (above) so components enabled externally still work.
            requested.push_back(result.space);
            if(XR_FAILED(requestResult)) {
                sceneLog("Scene component enable rejected (once)",static_cast<int>(requestResult));
                sceneLog("Rejected component type",static_cast<int>(type));
            }
            return false;
        };
        if(!enabled(XR_SPACE_COMPONENT_TYPE_SEMANTIC_LABELS_FB,requestedLabels_))continue;
        if(!enabled(XR_SPACE_COMPONENT_TYPE_BOUNDED_2D_FB,requestedBounds_))continue;
        if(!enabled(XR_SPACE_COMPONENT_TYPE_LOCATABLE_FB,requestedLocatable_))continue;
        XrSemanticLabelsFB labels{XR_TYPE_SEMANTIC_LABELS_FB};
        const XrResult labelCountResult=labels_(session_,result.space,&labels);
        if(XR_FAILED(labelCountResult) || labels.bufferCountOutput==0 ||
           labels.bufferCountOutput>1024) {
            static bool labelFailureLogged=false;
            if(!labelFailureLogged) {
                sceneLog("Scene label count status",static_cast<int>(labelCountResult));
                labelFailureLogged=true;
            }
            continue;
        }
        std::vector<char> buffer(labels.bufferCountOutput+1,'\0');
        labels.bufferCapacityInput=static_cast<uint32_t>(buffer.size());
        labels.buffer=buffer.data();
        if(XR_FAILED(labels_(session_,result.space,&labels)))continue;
        if(std::strstr(buffer.data(),"WALL_FACE")==nullptr ||
           std::strstr(buffer.data(),"INVISIBLE_WALL_FACE")!=nullptr)continue;
        XrRect2Df rect{};
        if(XR_FAILED(bounds_(session_,result.space,&rect)))continue;
        XrSpaceLocation loc{XR_TYPE_SPACE_LOCATION};
        if(XR_FAILED(xrLocateSpace(result.space,localSpace,time,&loc)))continue;
        constexpr auto required=XR_SPACE_LOCATION_POSITION_VALID_BIT|XR_SPACE_LOCATION_ORIENTATION_VALID_BIT;
        if((loc.locationFlags&required)!=required)continue;
        const auto& pose=loc.pose;
        XrVector3f localCenter{rect.offset.x+rect.extent.width*0.5f,
                                rect.offset.y+rect.extent.height*0.5f,0.f};
        const auto delta=rotate(pose.orientation,localCenter);
        voxen::scene::EnvironmentSurface wall{};
        uint64_t uuidHash=1469598103934665603ull;
        for(unsigned char byte:result.uuid.data) {
            uuidHash^=byte;
            uuidHash*=1099511628211ull;
        }
        wall.id=uuidHash;
        wall.classification=voxen::scene::SurfaceClass::Wall;
        wall.center={pose.position.x+delta.x,pose.position.y+delta.y,pose.position.z+delta.z};
        wall.orientation={pose.orientation.x,pose.orientation.y,pose.orientation.z,pose.orientation.w};
        wall.halfExtents={rect.extent.width*0.5f,rect.extent.height*0.5f};
        wall.tracked=true;
        walls.push_back(wall);
        if(walls.size()>=16)break;
    }
    if(walls.size()!=lastWallCount_) {
        lastWallCount_=static_cast<uint32_t>(walls.size());
        sceneLog("Tracked wall count",static_cast<int>(lastWallCount_));
    }
    surfaces_=std::move(walls);
}
void MetaSceneProvider::shutdown() {
    for(const auto& item:spaces_) if(item.space!=XR_NULL_HANDLE) xrDestroySpace(item.space);
    spaces_.clear();surfaces_.clear();pending_=false;requestId_=0;
    requestedLabels_.clear();requestedBounds_.clear();requestedLocatable_.clear();
    lastWallCount_=0;
    query_=nullptr;retrieve_=nullptr;bounds_=nullptr;labels_=nullptr;
    discover_=nullptr;retrieveDiscovery_=nullptr;usingDiscovery_=false;
    getStatus_=nullptr;setStatus_=nullptr;
    session_=XR_NULL_HANDLE;
}
} // namespace voxen::xr
