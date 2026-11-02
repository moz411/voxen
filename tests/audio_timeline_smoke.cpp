#include <voxen/audio/SpectrumAnalyzer.hpp>
#include <voxen/audio/PcmRingBuffer.hpp>
#include <voxen/animation_timeline.h>
#include <voxen/SceneClock.hpp>
#include <voxen/Choreography.hpp>
#include <array>
#include <cassert>
#include <cmath>
#include <vector>

int main() {
    using namespace voxen;
    audio::PcmRingBuffer<8> queue;
    const float source[] = {1.f, 2.f, 3.f, 4.f, 5.f, 6.f};
    float target[8]{};
    assert(queue.write(source, 6) == 6);
    assert(queue.read(target, 4) == 4);
    assert(target[0] == 1.f && target[3] == 4.f);
    assert(queue.write(source, 6) == 6);
    assert(queue.read(target, 8) == 8);
    assert(target[0] == 5.f && target[1] == 6.f && target[2] == 1.f);
    assert(queue.size() == 0);

    constexpr float pi = 3.14159265358979323846f;
    std::array<float, audio::SpectrumAnalyzer::Window> pcm{};
    for (std::size_t i = 0; i < pcm.size(); ++i)
        pcm[i] = 0.5f * std::sin(2.f * pi * 93.75f * static_cast<float>(i) / 48000.f);
    audio::SpectrumAnalyzer analyzer;
    const auto low = analyzer.analyze(pcm.data(), pcm.size(), 1, 48000);
    assert(low.rms > 0.3f);
    assert(low.bass > low.mid && low.bass > low.treble);
    assert(low.onset > 0.f);
    const auto steady = analyzer.analyze(pcm.data(), pcm.size(), 1, 48000);
    assert(steady.onset < 0.00001f);
    analyzer.reset();
    assert(analyzer.analyze(nullptr, 0, 1, 48000).rms == 0.f);

    animation::Choreography choreography;
    choreography.durationSeconds = 2.0;
    choreography.pingPong = true;
    animation::ChannelTrack position;
    position.channel = animation::Channel::PositionX;
    position.keys = {{0., 0.f}, {2., 10.f}};
    choreography.tracks.push_back(position);
    const animation::AnimatedValues base;
    const animation::AudioFeatures silence;
    assert(std::abs(choreography.evaluate(1., base, silence)[animation::Channel::PositionX] - 5.f) < 0.0001f);
    assert(std::abs(choreography.evaluate(3., base, silence)[animation::Channel::PositionX] - 5.f) < 0.0001f);
    assert(choreography.evaluate(4., base, silence)[animation::Channel::PositionX] == 0.f);
    assert(base[animation::Channel::PositionX] == 0.f);
    animation::SceneClock clock;
    clock.setLoop(4.0, true);
    clock.update(5.5);
    assert(std::abs(clock.seconds() - 1.5) < 0.0001);
    clock.syncToAudio(2.25);
    assert(std::abs(clock.seconds() - 2.25) < 0.0001);
    clock.pause(true);
    clock.update(1.0);
    assert(std::abs(clock.seconds() - 2.25) < 0.0001);
    clock.pause(false);
    const std::vector<animation::Keyframe> keys{
        {0., 0.f, animation::Interpolation::Linear},
        {2., 10.f, animation::Interpolation::Linear}
    };
    assert(std::abs(animation::sampleTrack(keys, 1.) - 5.f) < 0.0001f);
    animation::AudioFeatures features;
    features.bass = 0.5f;
    animation::AudioModulation mapping;
    mapping.gain = 2.f;
    mapping.maximum = 2.f;
    assert(std::abs(animation::composeParameter(3.f, keys, 1., features, mapping) - 6.f) < 0.0001f);
}
