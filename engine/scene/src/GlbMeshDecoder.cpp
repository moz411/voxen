#include "voxen/scene/GlbMeshDecoder.hpp"
#include "voxen/scene/GlbAsset.hpp"
#include <cgltf.h>
#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO
#include <stb_image.h>
#include <algorithm>
#include <climits>
#include <limits>
#include <memory>
#include <stdexcept>
#include <unordered_map>

namespace voxen::scene::gltf {
namespace {
void check(cgltf_result result, const char* operation) {
    if(result != cgltf_result_success)
        throw std::runtime_error(std::string("glTF: ") + operation + " failed (" +
                                 std::to_string(static_cast<int>(result)) + ")");
}
struct FreeData {
    void operator()(cgltf_data* p) const noexcept { if(p) cgltf_free(p); }
};
template<class T>
int indexOf(const T* item, const T* base, std::size_t count) {
    if(!item || !base) return -1;
    for(std::size_t i=0;i<count;++i) if(base+i==item) return static_cast<int>(i);
    return -1;
}
}

MeshAsset decodeMeshGlb(const std::vector<std::uint8_t>& bytes, const Limits& limits) {
    // Validate GLB framing before asking cgltf to parse the document.
    (void)decodeGlb(bytes, limits.maxFileBytes);
    cgltf_options options{};
    cgltf_data* raw=nullptr;
    check(cgltf_parse(&options, bytes.data(), bytes.size(), &raw), "parse");
    std::unique_ptr<cgltf_data,FreeData> data(raw);
    // Reject external buffers / images: v1 is a self-contained GLB importer.
    for(cgltf_size i=0;i<data->buffers_count;++i)
        if(data->buffers[i].uri)
            throw std::runtime_error("glTF: external buffer URI is unsupported");
    if(data->nodes_count>limits.maxNodes)
        throw std::runtime_error("glTF: node budget exceeded");
    check(cgltf_load_buffers(&options, data.get(), nullptr), "load embedded buffer");
    check(cgltf_validate(data.get()), "validate");

    MeshAsset output;
    output.images.reserve(data->images_count);
    for(cgltf_size i=0;i<data->images_count;++i) {
        EmbeddedImage image;
        const auto& src=data->images[i];
        if(src.buffer_view && src.buffer_view->buffer &&
           src.buffer_view->buffer->data && src.mime_type &&
           (std::string(src.mime_type)=="image/png" ||
            std::string(src.mime_type)=="image/jpeg") &&
           src.buffer_view->size<=limits.maxEmbeddedImageBytes) {
            const auto* base=static_cast<const std::uint8_t*>(src.buffer_view->buffer->data);
            const auto offset=src.buffer_view->offset;
            const auto length=src.buffer_view->size;
            if(offset<=src.buffer_view->buffer->size &&
               length<=src.buffer_view->buffer->size-offset) {
                image.encoded.assign(base+offset,base+offset+length);
                image.mimeType=src.mime_type;
                int w=0,h=0,channels=0;
                // Refuse giant decoded images before allocating RGBA memory.
                if(image.encoded.size()<=static_cast<size_t>(INT_MAX) &&
                   stbi_info_from_memory(image.encoded.data(),
                       static_cast<int>(image.encoded.size()),&w,&h,&channels) &&
                   w>0 && h>0 && w<=4096 && h<=4096 &&
                   static_cast<uint64_t>(w)*h<=16u*1024u*1024u) {
                    stbi_uc* rgba=stbi_load_from_memory(image.encoded.data(),
                        static_cast<int>(image.encoded.size()),&w,&h,&channels,4);
                    if(rgba) {
                        image.rgba.assign(rgba,rgba+static_cast<size_t>(w)*h*4);
                        image.width=w;image.height=h;
                        stbi_image_free(rgba);
                    }
                }
            }
        }
        output.images.push_back(std::move(image));
    }
    std::size_t totalVertices=0, totalIndices=0;
    std::unordered_map<const cgltf_mesh*,std::vector<std::uint32_t>> meshCache;
    auto loadMesh=[&](const cgltf_mesh* mesh)->const std::vector<std::uint32_t>& {
        const auto existing=meshCache.find(mesh);
        if(existing!=meshCache.end()) return existing->second;
        std::vector<std::uint32_t> refs;
        for(cgltf_size p=0;p<mesh->primitives_count;++p) {
            const auto& src=mesh->primitives[p];
            if(src.type!=cgltf_primitive_type_triangles)
                throw std::runtime_error("glTF: only TRIANGLES supported in v1");
            const cgltf_accessor *positions=nullptr,*normals=nullptr,*uvs=nullptr;
            for(cgltf_size a=0;a<src.attributes_count;++a) {
                const auto& attribute=src.attributes[a];
                if(attribute.type==cgltf_attribute_type_position)positions=attribute.data;
                else if(attribute.type==cgltf_attribute_type_normal)normals=attribute.data;
                else if(attribute.type==cgltf_attribute_type_texcoord && attribute.index==0)uvs=attribute.data;
            }
            if(!positions || positions->type!=cgltf_type_vec3)
                throw std::runtime_error("glTF: missing VEC3 POSITION");
            const std::size_t vertexCount=positions->count;
            // Older cgltf releases can mishandle zero-length accessors.
            if(vertexCount==0 || (src.indices && src.indices->count==0))
                throw std::runtime_error("glTF: empty geometry accessor");
            const std::size_t indexCount=src.indices?src.indices->count:vertexCount;
            if(vertexCount>limits.maxVertices-totalVertices ||
               indexCount>limits.maxIndices-totalIndices || indexCount%3)
                throw std::runtime_error("glTF: geometry budget / triangles invalid");
            if(normals && (normals->type!=cgltf_type_vec3 || normals->count!=vertexCount))
                throw std::runtime_error("glTF: inconsistent normals");
            if(uvs && (uvs->type!=cgltf_type_vec2 || uvs->count!=vertexCount))
                throw std::runtime_error("glTF: inconsistent UV");
            Primitive dst;
            dst.vertices.resize(vertexCount);
            dst.indices.reserve(indexCount);
            for(std::size_t v=0;v<vertexCount;++v) {
                auto& vertex=dst.vertices[v];
                if(!cgltf_accessor_read_float(positions,v,vertex.position.data(),3))
                    throw std::runtime_error("glTF: unreadable positions");
                if(normals && !cgltf_accessor_read_float(normals,v,vertex.normal.data(),3))
                    throw std::runtime_error("glTF: unreadable normals");
                if(uvs && !cgltf_accessor_read_float(uvs,v,vertex.uv.data(),2))
                    throw std::runtime_error("glTF: unreadable UV");
            }
            for(std::size_t j=0;j<indexCount;++j) {
                auto idx=src.indices?cgltf_accessor_read_index(src.indices,j):j;
                if(idx>=vertexCount) throw std::runtime_error("glTF: index out of bounds");
                dst.indices.push_back(static_cast<std::uint32_t>(idx));
            }
            if(src.material) {
                dst.materialIndex=indexOf(src.material,data->materials,data->materials_count);
                const auto& pbr=src.material->pbr_metallic_roughness;
                std::copy_n(pbr.base_color_factor,4,dst.baseColor.data());
                dst.metallic=pbr.metallic_factor;
                dst.roughness=pbr.roughness_factor;
                if(pbr.base_color_texture.texture &&
                   pbr.base_color_texture.texture->image &&
                   pbr.base_color_texture.texcoord==0)
                    dst.baseColorTexture=indexOf(pbr.base_color_texture.texture->image,
                                                 data->images,data->images_count);
            }
            refs.push_back(static_cast<std::uint32_t>(output.primitives.size()));
            output.primitives.push_back(std::move(dst));
            totalVertices+=vertexCount;
            totalIndices+=indexCount;
        }
        return meshCache.emplace(mesh,std::move(refs)).first->second;
    };

    // Only traverse the selected glTF scene (or the first scene), not orphan nodes.
    const cgltf_scene* scene=data->scene ? data->scene :
                               (data->scenes_count?&data->scenes[0]:nullptr);
    if(!scene) throw std::runtime_error("glTF: no scene");
    // Non-recursive traversal prevents stack exhaustion from hostile documents.
    struct Entry { const cgltf_node* node; int parent; };
    std::vector<Entry> pending;
    for(cgltf_size i=scene->nodes_count;i>0;--i)
        pending.push_back({scene->nodes[i-1],-1});
    while(!pending.empty()) {
        const auto item=pending.back(); pending.pop_back();
        if(output.nodes.size()>=limits.maxNodes)
            throw std::runtime_error("glTF: scene traversal budget exceeded");
        Node node;
        node.parent=item.parent;
        if(item.node->name) node.name=item.node->name;
        cgltf_node_transform_world(item.node,node.worldMatrix.data());
        if(item.node->mesh) node.primitives=loadMesh(item.node->mesh);
        const auto idx=static_cast<int>(output.nodes.size());
        output.nodes.push_back(std::move(node));
        for(cgltf_size i=item.node->children_count;i>0;--i)
            pending.push_back({item.node->children[i-1],idx});
    }
    return output;
}
MeshAsset loadMeshGlb(const std::filesystem::path& file,const Limits& limits) {
    // Prefer app-private, trusted asset paths supplied by the scene repository.
    const auto glb=loadGlb(file,limits.maxFileBytes);
    // The cgltf reader expects the complete GLB (including headers), not its
    // extracted BIN payload; read once again with the checked file limit.
    std::ifstream stream(file,std::ios::binary|std::ios::ate);
    if(!stream) throw std::runtime_error("glTF: cannot open file");
    const auto size=stream.tellg();
    if(size<0 || static_cast<std::uintmax_t>(size)>limits.maxFileBytes)
        throw std::runtime_error("glTF: invalid file size");
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(size));
    stream.seekg(0);
    if(!stream.read(reinterpret_cast<char*>(bytes.data()),static_cast<std::streamsize>(bytes.size())))
        throw std::runtime_error("glTF: truncated file");
    (void)glb;
    return decodeMeshGlb(bytes,limits);
}
} // namespace voxen::scene::gltf
