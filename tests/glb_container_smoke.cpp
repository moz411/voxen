#include "voxen/scene/GlbAsset.hpp"
#include <cassert>
#include <cstdint>
#include <functional>
#include <stdexcept>
#include <vector>
using namespace voxen::scene::gltf;

static void put32(std::vector<std::uint8_t>& a, std::uint32_t v) {
    for (int k=0;k<4;++k) a.push_back(static_cast<std::uint8_t>(v >> (k*8)));
}
static std::vector<std::uint8_t> fixture() {
    std::vector<std::uint8_t> a;
    put32(a,0x46546c67); put32(a,2); put32(a,0);
    const char json[] = "{\"asset\":{\"version\":\"2.0\"}}";
    std::vector<std::uint8_t> j(json, json + sizeof(json)-1);
    while(j.size()%4)j.push_back(' ');
    put32(a,static_cast<std::uint32_t>(j.size()));put32(a,0x4e4f534a);
    a.insert(a.end(),j.begin(),j.end());
    put32(a,4);put32(a,0x004e4942);
    a.insert(a.end(),{1,2,3,4});
    const auto size=static_cast<std::uint32_t>(a.size());
    for(int k=0;k<4;++k)a[8+k]=static_cast<std::uint8_t>(size>>(k*8));
    return a;
}
static bool rejects(const std::vector<std::uint8_t>& bytes) {
    try { (void)decodeGlb(bytes); } catch (const std::runtime_error&) { return true; }
    return false;
}
int main() {
    auto a=fixture();
    auto decoded=decodeGlb(a);
    assert(decoded.json=="{\"asset\":{\"version\":\"2.0\"}}");
    assert((decoded.binary==std::vector<std::uint8_t>{1,2,3,4}));
    assert(rejects({}));
    assert(rejects(std::vector<std::uint8_t>(a.begin(),a.end()-1)));
    try { (void)decodeGlb(a, 16); assert(false); } catch (const std::runtime_error&) {}\n    a[4]=1; assert(rejects(a));
    return 0;
}
