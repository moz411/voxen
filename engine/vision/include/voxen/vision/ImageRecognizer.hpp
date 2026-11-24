#pragma once
#include <opencv2/core.hpp>
#include <opencv2/features2d.hpp>
#include <opencv2/calib3d.hpp>
#include <opencv2/imgproc.hpp>
#include <array>
#include <string>
#include <vector>

namespace voxen::vision {
struct Detection {
    std::string id;
    std::array<cv::Point2f,4> corners;
    int inliers;
};
class ImageRecognizer {
    struct Reference {
        std::string id;
        cv::Size size;
        std::vector<cv::KeyPoint> points;
        cv::Mat descriptors;
    };
    cv::Ptr<cv::ORB> orb_ = cv::ORB::create(800);
    std::vector<Reference> refs_;
public:
    // Use from a single worker thread; input is grayscale CV_8UC1.
    bool add(std::string id, const cv::Mat& gray) {
        if(gray.empty() || gray.type()!=CV_8UC1) return false;
        Reference r{std::move(id),gray.size(),{},{}};
        orb_->detectAndCompute(gray,cv::noArray(),r.points,r.descriptors);
        if(r.descriptors.rows<20) return false;
        refs_.push_back(std::move(r));
        return true;
    }
    std::vector<Detection> detect(const cv::Mat& gray) {
        std::vector<Detection> out;
        if(gray.empty() || gray.type()!=CV_8UC1) return out;
        std::vector<cv::KeyPoint> points;
        cv::Mat desc;
        orb_->detectAndCompute(gray,cv::noArray(),points,desc);
        if(desc.rows<2) return out;
        cv::BFMatcher matcher(cv::NORM_HAMMING);
        for(const auto& r:refs_) {
            std::vector<std::vector<cv::DMatch>> matches;
            matcher.knnMatch(r.descriptors,desc,matches,2);
            std::vector<cv::Point2f> a,b;
            for(const auto& m:matches) if(m.size()==2 && m[0].distance<0.75f*m[1].distance) {
                a.push_back(r.points[m[0].queryIdx].pt);
                b.push_back(points[m[0].trainIdx].pt);
            }
            if(a.size()<15) continue;
            cv::Mat mask;
            auto h=cv::findHomography(a,b,cv::RANSAC,3.0,mask);
            if(h.empty()) continue;
            int n=cv::countNonZero(mask);
            if(n<15 || n<0.5*a.size()) continue;
            std::vector<cv::Point2f> src{{0,0},{float(r.size.width-1),0},{float(r.size.width-1),float(r.size.height-1)},{0,float(r.size.height-1)}}, dst;
            cv::perspectiveTransform(src,dst,h);
            if(!cv::checkRange(cv::Mat(dst)) || !cv::isContourConvex(dst) || cv::contourArea(dst)<400) continue;
            out.push_back({r.id,{dst[0],dst[1],dst[2],dst[3]},n});
        }
        return out;
    }
};
}
