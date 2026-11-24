#include "voxen/vision/ImageRecognizer.hpp"
#include <opencv2/imgcodecs.hpp>
#include <filesystem>
namespace voxen::vision {
std::size_t loadReferences(ImageRecognizer& recognizer, const std::filesystem::path& root) {
    std::error_code ec;
    std::size_t count=0;
    for(std::filesystem::directory_iterator it(root,ec), end; !ec && it!=end; it.increment(ec)) {
        if(count>=10) break;
        if(!it->is_regular_file(ec)) continue;
        const auto p=it->path();
        const auto ext=p.extension().string();
        if(ext!=".jpg" && ext!=".jpeg" && ext!=".png") continue;
        auto img=cv::imread(p.string(),cv::IMREAD_GRAYSCALE);
        if(recognizer.add(p.stem().string(),img)) ++count;
    }
    return count;
}
}
