#include <voxen/render/PanoramaImage.hpp>
#include <stb_image.h>
#include <stdexcept>
#include <limits>
#include <fstream>
#include <iterator>
#include <algorithm>
#include <cmath>

namespace voxen::render {
PanoramaImage decodePanoramaImage(const std::filesystem::path& file,
                                  int maxDimension,std::uint64_t maxPixels) {
    // Decoding a JPEG requires full source pixels with stb_image. Keep a hard
    // temporary CPU-memory ceiling independent from the GPU texture budget.
    constexpr std::uint64_t maxSourcePixels=64ull*1024ull*1024ull;
    if(!std::filesystem::is_regular_file(file) || std::filesystem::is_symlink(file))
        throw std::runtime_error("Panorama: invalid file");
    const auto bytes=std::filesystem::file_size(file);
    if(bytes==0 || bytes>64ull*1024ull*1024ull)
        throw std::runtime_error("Panorama: invalid file size");
    if(maxDimension<1 || maxPixels==0)
        throw std::runtime_error("Panorama: invalid texture budget");
    std::ifstream input(file,std::ios::binary);
    if(!input)throw std::runtime_error("Panorama: cannot read file");
    std::vector<stbi_uc> encoded((std::istreambuf_iterator<char>(input)),
                                  std::istreambuf_iterator<char>());
    if(encoded.size()!=bytes)throw std::runtime_error("Panorama: truncated read");
    int width=0,height=0,components=0;
    if(!stbi_info_from_memory(encoded.data(),static_cast<int>(encoded.size()),&width,&height,&components))
        throw std::runtime_error("Panorama: unsupported or corrupt image");
    if(width<=0 || height<=0 || static_cast<std::uint64_t>(width)*height>maxSourcePixels)
        throw std::runtime_error("Panorama: source exceeds 64 megapixel CPU safety limit");
    double scale=std::min({1.0,
        static_cast<double>(maxDimension)/width,
        static_cast<double>(maxDimension)/height,
        std::sqrt(static_cast<double>(maxPixels)/(static_cast<double>(width)*height))});
    int targetWidth=std::max(1,static_cast<int>(std::floor(width*scale)));
    int targetHeight=std::max(1,static_cast<int>(std::floor(height*scale)));
    int decodedWidth=0,decodedHeight=0,decodedComponents=0;
    stbi_uc* data=stbi_load_from_memory(encoded.data(),static_cast<int>(encoded.size()),
                                &decodedWidth,&decodedHeight,&decodedComponents,STBI_rgb_alpha);
    if(!data)throw std::runtime_error("Panorama: decode failed");
    if(decodedWidth!=width || decodedHeight!=height) {
        stbi_image_free(data);
        throw std::runtime_error("Panorama: dimensions changed during decode");
    }
    PanoramaImage result;
    result.width=targetWidth;
    result.height=targetHeight;
    try {
        const auto length=static_cast<std::size_t>(targetWidth)*targetHeight*4u;
        result.rgba.resize(length);
        if(targetWidth==width && targetHeight==height) {
            std::copy_n(data,length,result.rgba.data());
        } else {
            // Bilinear downsampling, keeping aspect ratio and limiting VRAM.
            // No extra full-size intermediate image is allocated.
            for(int y=0;y<targetHeight;++y) {
                const float fy=(static_cast<float>(y)+0.5f)*height/targetHeight-0.5f;
                const int y0=std::clamp(static_cast<int>(std::floor(fy)),0,height-1);
                const int y1=std::min(y0+1,height-1);
                const float ty=std::clamp(fy-y0,0.f,1.f);
                for(int x=0;x<targetWidth;++x) {
                    const float fx=(static_cast<float>(x)+0.5f)*width/targetWidth-0.5f;
                    const int x0=std::clamp(static_cast<int>(std::floor(fx)),0,width-1);
                    const int x1=std::min(x0+1,width-1);
                    const float tx=std::clamp(fx-x0,0.f,1.f);
                    const auto a=(static_cast<std::size_t>(y0)*width+x0)*4;
                    const auto b=(static_cast<std::size_t>(y0)*width+x1)*4;
                    const auto c=(static_cast<std::size_t>(y1)*width+x0)*4;
                    const auto d=(static_cast<std::size_t>(y1)*width+x1)*4;
                    const auto dst=(static_cast<std::size_t>(y)*targetWidth+x)*4;
                    for(int channel=0;channel<4;++channel) {
                        const float top=data[a+channel]*(1-tx)+data[b+channel]*tx;
                        const float bottom=data[c+channel]*(1-tx)+data[d+channel]*tx;
                        result.rgba[dst+channel]=static_cast<std::uint8_t>(
                            std::clamp(top*(1-ty)+bottom*ty,0.f,255.f)+0.5f);
                    }
                }
            }
        }
    } catch(...) {
        stbi_image_free(data);
        throw;
    }
    stbi_image_free(data);
    return result;
}
}
