#include <voxen/render/OpenBrushDots.hpp>
#include <cassert>
#include <cmath>
#include <limits>
int main() {
    using namespace voxen::render::openbrush;
    std::vector<ControlPoint> points{{{1.f, 2.f, 3.f}, 0.5f},
                                     {{2.f, 3.f, 4.f}, 1.f},
                                     {{0.f, 0.f, 0.f}, std::numeric_limits<float>::quiet_NaN()}};
    auto dots = buildDots(points, {0.2f, 0.5f, 0.8f, 1.f}, 0.1f, 2.f, 0.5f);
    assert(dots.size() == 2);
    assert(std::abs(dots[0].diameterMeters - 0.05f) < 0.00001f);
    assert(std::abs(dots[1].center[0] - 1.f) < 0.00001f);
    assert(buildDots(points, {1,1,1,1}, -1.f).empty());
}
