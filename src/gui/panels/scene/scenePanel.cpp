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

    if (m_anim < 0.95f) drawEdgeHint(io, mouseNear);
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

    // Кастомный accent bar на правом краю панели
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 winPos = ImGui::GetWindowPos();
    ImVec2 winSize = ImGui::GetWindowSize();

    drawList->AddRectFilledMultiColor(
        ImVec2(winPos.x + winSize.x - 3.0f, winPos.y),
        ImVec2(winPos.x + winSize.x, winPos.y + winSize.y),
        IM_COL32(60, 120, 200, 255),
        IM_COL32(60, 120, 200, 255),
        IM_COL32(40, 80, 160, 255),
        IM_COL32(40, 80, 160, 255)
    );

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

void ScenePanel::drawEdgeHint(const ImGuiIO& io, bool mouseNear)
{
    const float baseAlpha = 1.0f - m_anim;
    if (baseAlpha < 0.01f) return;

    // Пульсация для привлечения внимания
    const float time = (float)ImGui::GetTime();
    const float pulse = 0.85f + 0.15f * sinf(time * 2.0f);
    const float alpha = baseAlpha * pulse;

    // Цвет меняется при наведении
    const ImVec4 normalColor = ImVec4(0.70f, 0.85f, 1.00f, alpha);
    const ImVec4 hoverColor = ImVec4(0.90f, 0.95f, 1.00f, alpha);
    const ImVec4 textColor = mouseNear ? hoverColor : normalColor;

    const float centerY = io.DisplaySize.y * 0.5f;

    ImGui::SetNextWindowPos(
        ImVec2(io.DisplaySize.x - kHintW, centerY - kHintH * 0.5f),
        ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(kHintW, kHintH), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(alpha * 0.7f);

    ImGui::Begin("SceneHint", nullptr,
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoInputs     |
        ImGuiWindowFlags_NoNav        |
        ImGuiWindowFlags_NoMove       |
        ImGuiWindowFlags_NoSavedSettings);

    // Кастомная отрисовка закругленного таба
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 winPos = ImGui::GetWindowPos();
    ImVec2 winSize = ImGui::GetWindowSize();

    // Градиентный фон таба
    drawList->AddRectFilledMultiColor(
        winPos,
        ImVec2(winPos.x + winSize.x, winPos.y + winSize.y),
        IM_COL32(40, 80, 130, (int)(alpha * 200)),
        IM_COL32(30, 60, 100, (int)(alpha * 180)),
        IM_COL32(30, 60, 100, (int)(alpha * 180)),
        IM_COL32(40, 80, 130, (int)(alpha * 200))
    );

    // Accent линия слева
    drawList->AddRectFilled(
        ImVec2(winPos.x, winPos.y),
        ImVec2(winPos.x + 2.0f, winPos.y + winSize.y),
        IM_COL32(60, 120, 200, (int)(alpha * 255))
    );

    // Иконка шеврона
    ImGui::SetCursorPosX(kHintW * 0.5f - 8.0f);
    ImGui::SetCursorPosY(kHintH * 0.5f - 8.0f);
    ImGui::TextColored(textColor, "<<");

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