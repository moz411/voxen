#include <voxen/render/PanoramaImage.hpp>
#include <stb_image.h>
#include <stdexcept>
#include <limits>

namespace voxen::render {
PanoramaImage decodePanoramaImage(const std::filesystem::path& file) {
    constexpr int maxDimension=8192;
    constexpr std::uint64_t maxPixels=8192ull*4096ull;
    if(!std::filesystem::is_regular_file(file) || std::filesystem::is_symlink(file))
        throw std::runtime_error("Panorama: invalid file");
    const auto bytes=std::filesystem::file_size(file);
    if(bytes==0 || bytes>64ull*1024ull*1024ull)
        throw std::runtime_error("Panorama: invalid file size");
    const auto path=file.string();
    int width=0,height=0,components=0;
    if(!stbi_info(path.c_str(),&width,&height,&components))
        throw std::runtime_error("Panorama: unsupported or corrupt image");
    if(width<=0 || height<=0 || width>maxDimension || height>maxDimension ||
       static_cast<std::uint64_t>(width)*height>maxPixels)
        throw std::runtime_error("Panorama: image dimensions exceed limit");
    int decodedWidth=0,decodedHeight=0,decodedComponents=0;
    stbi_uc* data=stbi_load(path.c_str(),&decodedWidth,&decodedHeight,
                             &decodedComponents,STBI_rgb_alpha);
    if(!data)throw std::runtime_error("Panorama: decode failed");
    PanoramaImage result;
    if(decodedWidth!=width || decodedHeight!=height) {
        stbi_image_free(data);
        throw std::runtime_error("Panorama: dimensions changed during decode");
    }
    result.width=width;result.height=height;
    const auto length=static_cast<std::size_t>(width)*height*4u;
    result.rgba.assign(data,data+length);
    stbi_image_free(data);
    return result;
}
}
