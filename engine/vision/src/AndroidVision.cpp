#include "voxen/vision/ImageRecognizer.hpp"
#include <jni.h>
#include <android/log.h>
#include <filesystem>
#include <memory>
namespace voxen::vision {
std::size_t loadReferences(ImageRecognizer&, const std::filesystem::path&);
}
static std::unique_ptr<voxen::vision::ImageRecognizer> recognizer;
extern "C" JNIEXPORT jint JNICALL Java_org_voxen_xr_VisionCamera_nativeInit(JNIEnv* env,jclass,jstring path) {
    const char* p=env->GetStringUTFChars(path,nullptr);
    if(!p) return 0;
    std::filesystem::path root(p);
    env->ReleaseStringUTFChars(path,p);
    try {
        recognizer=std::make_unique<voxen::vision::ImageRecognizer>();
        auto n=voxen::vision::loadReferences(*recognizer,root);
        __android_log_print(ANDROID_LOG_INFO,"VoxenVision","Loaded %zu references",n);
        return static_cast<jint>(n);
    } catch(const std::exception& e) {
        __android_log_print(ANDROID_LOG_ERROR,"VoxenVision","Init: %s",e.what());
        return 0;
    }
}
extern "C" JNIEXPORT void JNICALL Java_org_voxen_xr_VisionCamera_nativeFrame(JNIEnv* env,jclass,jbyteArray pixels,jint w,jint h) {
    if(!recognizer || w<=0 || h<=0 || w>4096 || h>4096 || env->GetArrayLength(pixels)!=w*h) return;
    try {
        cv::Mat gray(h,w,CV_8UC1);
        env->GetByteArrayRegion(pixels,0,w*h,reinterpret_cast<jbyte*>(gray.data));
        if(env->ExceptionCheck()) return;
        for(const auto& d:recognizer->detect(gray))
            __android_log_print(ANDROID_LOG_INFO,"VoxenVision","Detected %s inliers=%d",d.id.c_str(),d.inliers);
    } catch(const std::exception& e) {
        __android_log_print(ANDROID_LOG_ERROR,"VoxenVision","Frame: %s",e.what());
    }
}
