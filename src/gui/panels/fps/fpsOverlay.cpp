#include "fpsOverlay.h"
#include <imgui.h>

void FpsOverlay::draw(const FrameTimer& timer)
{
    const ImGuiIO& io = ImGui::GetIO();
    const float margin = 10.0f;

    // FPS счётчик 
    ImGui::SetNextWindowPos(
        ImVec2(io.DisplaySize.x - margin, margin),
        ImGuiCond_Always,
        ImVec2(1.0f, 0.0f)
    );
    ImGui::SetNextWindowBgAlpha(0.55f);
    ImGui::SetNextWindowSize(ImVec2(0, 0));

    ImGui::Begin("##FpsOverlay", nullptr,
        ImGuiWindowFlags_NoDecoration    |
        ImGuiWindowFlags_NoInputs        |
        ImGuiWindowFlags_NoNav           |
        ImGuiWindowFlags_NoMove          |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_AlwaysAutoResize
    );

    const float fps = timer.avgFps();
    ImVec4 color;
    if (fps >= 90.0f) color = ImVec4(0.35f, 0.95f, 0.45f, 1.0f);
    else if (fps >= 60.0f) color = ImVec4(0.95f, 0.85f, 0.20f, 1.0f);
    else color = ImVec4(0.95f, 0.30f, 0.25f, 1.0f);

    ImGui::TextColored(color, "%.0f FPS", fps);
    ImGui::TextDisabled("%d particles", m_particleCount);

    ImGui::End();

    const bool statsVisible = (io.MousePos.x > io.DisplaySize.x - 160.0f) && (io.MousePos.y < 90.0f);
    if (!statsVisible)
    {
        ImGui::SetNextWindowPos(
            ImVec2(io.DisplaySize.x - margin, margin + 46.0f),
            ImGuiCond_Always,
            ImVec2(1.0f, 0.0f)
        );
        ImGui::SetNextWindowBgAlpha(0.30f);
        ImGui::SetNextWindowSize(ImVec2(0, 0));

        ImGui::Begin("##StatsHint", nullptr,
            ImGuiWindowFlags_NoDecoration    |
            ImGuiWindowFlags_NoInputs        |
            ImGuiWindowFlags_NoNav           |
            ImGuiWindowFlags_NoMove          |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_AlwaysAutoResize
        );
        ImGui::TextDisabled("[ hover for timing ]");
        ImGui::End();
    }
}