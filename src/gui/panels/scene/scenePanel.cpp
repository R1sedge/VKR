#include "scenePanel.h"

#include "app/appState.h"
#include "app/appCommands.h"
#include "scene/ScenePresets.h"

#include "imgui.h"
#include <cmath>
#include <algorithm>

void ScenePanel::draw(const AppState& state, AppCommands& commands) 
{
    const ImGuiIO& io = ImGui::GetIO();
    const float dt = io.DeltaTime;

    // Зона триггера: правый край экрана
    const float visibleEdge = io.DisplaySize.x - kPanelW * m_anim;
    const float triggerEdge = std::min(io.DisplaySize.x - kTriggerX, visibleEdge);
    const bool mouseNear = io.MousePos.x >= triggerEdge;

    const float target = mouseNear ? 1.0f : 0.0f;

    m_anim += (target - m_anim) * (1.0f - std::exp(-kAnimSpeed * dt));

    if (m_anim < 0.95f) drawEdgeHint(io);
    if (m_anim < 0.002f) return;

    const float xPos = io.DisplaySize.x - kPanelW * m_anim;
    const float panelH = io.DisplaySize.y / 2.0f;

    ImGui::SetNextWindowPos(ImVec2(xPos, io.DisplaySize.y / 4.0f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(kPanelW, panelH), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.88f);

    ImGui::Begin("ScenePanel", nullptr,
        ImGuiWindowFlags_NoDecoration    |
        ImGuiWindowFlags_NoMove          |
        ImGuiWindowFlags_NoResize        |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoFocusOnAppearing);

    ImGui::Spacing();
    const float titleW = ImGui::CalcTextSize("Scenes").x;
    ImGui::SetCursorPosX((kPanelW - titleW) * 0.5f);
    ImGui::TextColored(ImVec4(0.60f, 0.80f, 1.00f, 1.0f), "Scenes");
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    drawSceneList(state, commands);

    ImGui::End();
}

void ScenePanel::drawEdgeHint(const ImGuiIO& io)  //TODO - переделать на иконку
{
    const float alpha = 1.0f - m_anim;
    if (alpha < 0.01f) return;

    const float h = 22.0f;
    const float centerY = io.DisplaySize.y * 0.5f;

    ImGui::SetNextWindowPos(
        ImVec2(io.DisplaySize.x - 14.0f, centerY - h * 0.5f),
        ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(14.0f, h), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(alpha * 0.55f);

    ImGui::Begin("SceneHint", nullptr,
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoInputs     |
        ImGuiWindowFlags_NoNav        |
        ImGuiWindowFlags_NoMove       |
        ImGuiWindowFlags_NoSavedSettings);

    ImGui::TextColored(ImVec4(0.70f, 0.85f, 1.00f, alpha * 0.70f), "<");

    ImGui::End();
}

void ScenePanel::drawSceneList(const AppState& state, AppCommands& commands) 
{
    const float innerW = kPanelW - 16.0f;
    const char* const* names = ScenePresets::names();
    const int count = ScenePresets::count();

    for (int i = 0; i < count; i++) 
    {
        const bool selected = (i == m_sceneIndex);

        // Активная сцена — акцентная кнопка, остальные — нейтральные
        if (selected) 
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.45f, 0.75f, 0.85f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.28f, 0.55f, 0.88f, 1.00f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.38f, 0.65f, 1.00f, 1.00f));
        } 
        else 
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.14f, 0.14f, 0.18f, 0.80f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.22f, 0.22f, 0.30f, 1.00f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.28f, 0.28f, 0.38f, 1.00f));
        }

        if (ImGui::Button(names[i], ImVec2(innerW, 0.0f))) 
        {
            m_sceneIndex = i;
            commands.hasSetScene = true;
            commands.sceneIndex = i;
        }

        ImGui::PopStyleColor(3);
        ImGui::Spacing();
    }
}