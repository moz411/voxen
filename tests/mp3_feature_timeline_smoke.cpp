#include <voxen/audio/Mp3FeatureTimeline.hpp>
#include <cassert>
#include <cmath>
int main() {
    voxen::audio::Mp3FeatureTimeline timeline;
    assert(!timeline.ready());
    const auto empty = timeline.at(0.0);
    assert(empty.rms == 0.f && empty.bass == 0.f);
    timeline.start("");
    assert(!timeline.ready());
#ifndef __ANDROID__
    timeline.start("/nonexistent/test.mp3");
    timeline.stop();
    assert(!timeline.ready());
#endif
    assert(timeline.at(NAN).rms == 0.f);
}
