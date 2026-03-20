#include "statsPanel.h"
#include <imgui.h>


void StatsPanel::timingRow(const char* label, float ms, float colOffset)
{
    ImGui::TextDisabled("%s", label);
    ImGui::SameLine(colOffset);
    ImGui::Text("%.3f ms", ms);
}

void StatsPanel::draw(const FrameTimer& timer, float simDt)
{
    const ImGuiIO& io = ImGui::GetIO();

    // Показываем только если мышь в зоне триггера
    const bool inZone = (io.MousePos.x > io.DisplaySize.x - kTriggerW) &&
                        (io.MousePos.y < kTriggerH);
    if (!inZone) return;

    const float margin = 10.0f;
    ImGui::SetNextWindowPos(
        ImVec2(io.DisplaySize.x - margin, kTopOffset),
        ImGuiCond_Always,
        ImVec2(1.0f, 0.0f)
    );
    ImGui::SetNextWindowBgAlpha(0.80f);

    ImGui::Begin("##StatsPanel", nullptr,
        ImGuiWindowFlags_NoDecoration    |
        ImGuiWindowFlags_NoInputs        |
        ImGuiWindowFlags_NoNav           |
        ImGuiWindowFlags_NoMove          |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_AlwaysAutoResize
    );

    ImGui::TextDisabled("Timing  (avg %d frames)", FrameTimer::kSize);
    ImGui::Separator();

    timingRow("Frame  ", timer.avgFrameMs());
    timingRow("Physics", timer.avgPhysicsMs());
    timingRow("Render ", timer.avgRenderMs());

    ImGui::Separator();
    ImGui::TextDisabled("Sim dt");
    ImGui::SameLine(120.0f);
    ImGui::Text("%.4f s", simDt);

    ImGui::End();
}