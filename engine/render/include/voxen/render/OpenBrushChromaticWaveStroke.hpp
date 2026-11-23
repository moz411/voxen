#pragma once
#include <voxen/render/OpenBrushNeonTrailsStroke.hpp>

namespace voxen::render::openbrush {
inline constexpr int ChromaticWaveStrokePreset=36;
inline constexpr const char* ChromaticWaveBrushGuid="voxen-native-chromatic-wave";
inline CompiledScene makeChromaticWaveStrokeScene(const std::vector<SmokeStrokePoint>& points) {
    auto scene=makeNeonTrailsStrokeScene(points);
    for(auto& mesh:scene.meshes)mesh.brushGuid=ChromaticWaveBrushGuid;
    return scene;
}
}
