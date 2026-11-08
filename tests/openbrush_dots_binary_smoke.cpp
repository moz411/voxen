#include <voxen/render/OpenBrushDotsBinary.hpp>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <stdexcept>
int main() {
    using namespace voxen::render::openbrush;
    const std::uint8_t data[] = {
        'V','X','D','O','T','0','0','1',
        1,0,0,0, 32,0,0,0,
        0,0,128,63, 0,0,0,64, 0,0,64,64,
        0,0,128,63, 0,0,0,0, 0,0,0,0, 0,0,128,63,
        0,0,0,63
    };
    auto dots = decodeDotsBinary(data, sizeof(data));
    assert(dots.size() == 1);
    assert(std::abs(dots[0].center[2] - 3.f) < 1e-6f);
    assert(std::abs(dots[0].diameterMeters - 0.5f) < 1e-6f);
    bool rejected = false;
    try { decodeDotsBinary(data, sizeof(data)-1); }
    catch (const std::runtime_error&) { rejected = true; }
    assert(rejected);
}
