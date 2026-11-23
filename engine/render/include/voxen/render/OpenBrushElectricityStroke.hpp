#pragma once
#include <voxen/render/OpenBrushLightningStroke.hpp>

namespace voxen::render::openbrush {
inline constexpr int ElectricityStrokePreset=37;
inline constexpr const char* ElectricityBrushGuid="voxen-native-electricity";
inline CompiledScene makeElectricityStrokeScene(const std::vector<SmokeStrokePoint>& points) {
    auto scene=makeLightningStrokeScene(points);
    for(auto& mesh:scene.meshes)mesh.brushGuid=ElectricityBrushGuid;
    return scene;
}
}
