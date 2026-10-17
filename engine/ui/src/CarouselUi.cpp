#include <voxen/ui/CarouselUi.hpp>
#include <imgui.h>
#include <algorithm>
#include <cstdio>

namespace voxen::ui {
void drawCarousel(CarouselState& state) {
    if (!state.visible) return;
    state.page=std::clamp(state.page,0,7);
    ImGui::SetNextWindowPos(ImVec2(8,8),ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(500,660),ImGuiCond_Always);
    constexpr ImGuiWindowFlags flags=ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;
    if (ImGui::Begin("VOXEN",nullptr,flags)) {
        if (ImGui::Button("< PREV")) state.page=(state.page+7)%8;
        ImGui::SameLine();
        ImGui::Text("%.*s",static_cast<int>(pages[state.page].size()),pages[state.page].data());
        ImGui::SameLine();
        if (ImGui::Button("NEXT >")) state.page=(state.page+1)%8;
        ImGui::Separator();
        switch(state.page) {
        case 3:
            ImGui::TextUnformatted("GPU MATERIALS");
            ImGui::TextDisabled("Shader library migration pending");
            break;
        case 4:
            ImGui::SliderFloat("Animation speed",&state.animationSpeed,0.1f,3.0f);
            ImGui::Checkbox("Performance HUD",&state.perfHud);
            break;
        case 7:
            ImGui::Checkbox("Liquid Room",&state.liquidRoom);
            ImGui::SliderFloat("Opacity",&state.liquidOpacity,0.0f,0.6f);
            ImGui::TextDisabled("Surface detection is not connected");
            break;
        default:
            ImGui::TextDisabled("Existing StereoKit controls: port pending");
            break;
        }
        ImGui::Separator();
        if(state.perfHud) {
            ImGui::Text("APP CPU  %.2f ms",state.cpuMs);
            if(state.gpuAvailable) ImGui::Text("GPU  %.2f ms",state.gpuMs);
            else ImGui::TextDisabled("GPU  N/A");
        }
    }
    ImGui::End();
}
} // namespace voxen::ui
