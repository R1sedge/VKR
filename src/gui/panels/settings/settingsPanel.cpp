#include "settingsPanel.h"

#include "app/appState.h"
#include "app/appCommands.h"
#include "render/Camera3D.h"

#include <imgui.h>
#include <cmath>
#include <algorithm>

void SettingsPanel::draw(const AppState& state, const Camera3D& camera, AppCommands& commands)
{
    const ImGuiIO& io = ImGui::GetIO();
    const float dt = io.DeltaTime;

    // Держим панель открытой если:
    //  - курсор в зоне триггера у левого края, ИЛИ
    //  - ImGui сейчас обрабатывает активный элемент (слайдер захвачен, кнопка зажата)
    const float visibleEdge = kPanelW * m_anim;  // реальный правый край панели
    const float triggerEdge = std::max(kTriggerX, visibleEdge);
    const bool mouseNear = io.MousePos.x < triggerEdge;

    const float target = mouseNear ? 1.0f : 0.0f;

    m_anim += (target - m_anim) * (1.0f - std::exp(-kAnimSpeed * dt));

    // Рисуем хинт у левого края когда панель скрыта
    if (m_anim < 0.95f)
        drawEdgeHint(io, mouseNear);

    if (m_anim < 0.002f) return;

    // Позиция: выезжает из-за левого края
    const float xPos = -kPanelW + kPanelW * m_anim;
    const float panelH = io.DisplaySize.y;

    ImGui::SetNextWindowPos(ImVec2(xPos, 0.0f),      ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(kPanelW, panelH), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.88f);

    ImGui::Begin("##SettingsPanel", nullptr,
        ImGuiWindowFlags_NoDecoration    |
        ImGuiWindowFlags_NoMove          |
        ImGuiWindowFlags_NoResize        |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoFocusOnAppearing
    );

    // Кастомный accent bar на левом краю панели
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 winPos = ImGui::GetWindowPos();
    ImVec2 winSize = ImGui::GetWindowSize();

    drawList->AddRectFilledMultiColor(
        winPos,
        ImVec2(winPos.x + 3.0f, winPos.y + winSize.y),
        IM_COL32(60, 120, 200, 255),
        IM_COL32(60, 120, 200, 255),
        IM_COL32(40, 80, 160, 255),
        IM_COL32(40, 80, 160, 255)
    );

    ImGui::Spacing();
    {
        const float titleW = ImGui::CalcTextSize("Settings").x;
        ImGui::SetCursorPosX((kPanelW - titleW) * 0.5f);
        ImGui::TextColored(ImVec4(0.60f, 0.80f, 1.00f, 1.0f), "Settings");
    }
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    drawPbfSection(commands);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    drawSimSection(state, commands);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    drawInteractionSection(state, commands);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    drawCameraSection(camera, commands);

    ImGui::Spacing();
    ImGui::End();
}

//  Хинт у левого края (виден когда панель убрана)
void SettingsPanel::drawEdgeHint(const ImGuiIO& io, bool mouseNear)
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

    ImGui::SetNextWindowPos(ImVec2(0.0f, centerY - kHintH * 0.5f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(kHintW, kHintH), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(alpha * 0.7f);

    ImGui::Begin("##SettingsHint", nullptr,
        ImGuiWindowFlags_NoDecoration    |
        ImGuiWindowFlags_NoInputs        |
        ImGuiWindowFlags_NoNav           |
        ImGuiWindowFlags_NoMove          |
        ImGuiWindowFlags_NoSavedSettings
    );

    // Кастомная отрисовка закругленного таба
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 winPos = ImGui::GetWindowPos();
    ImVec2 winSize = ImGui::GetWindowSize();

    // Градиентный фон таба
    drawList->AddRectFilledMultiColor(
        winPos,
        ImVec2(winPos.x + winSize.x, winPos.y + winSize.y),
        IM_COL32(30, 60, 100, (int)(alpha * 180)),
        IM_COL32(40, 80, 130, (int)(alpha * 200)),
        IM_COL32(40, 80, 130, (int)(alpha * 200)),
        IM_COL32(30, 60, 100, (int)(alpha * 180))
    );

    // Accent линия справа
    drawList->AddRectFilled(
        ImVec2(winPos.x + winSize.x - 2.0f, winPos.y),
        ImVec2(winPos.x + winSize.x, winPos.y + winSize.y),
        IM_COL32(60, 120, 200, (int)(alpha * 255))
    );

    // Иконка шеврона
    ImGui::SetCursorPosX(kHintW * 0.5f - 8.0f);
    ImGui::SetCursorPosY(kHintH * 0.5f - 8.0f);
    ImGui::TextColored(textColor, ">>");

    ImGui::End();
}

//  PBF секция
void SettingsPanel::drawPbfSection(AppCommands& commands)
{
    const float innerW = kPanelW - 20.0f;

    ImGui::TextColored(ImVec4(0.55f, 0.75f, 1.00f, 1.0f), "PBF Fluid");
    ImGui::Spacing();

    ImGui::SetNextItemWidth(innerW);
    if (ImGui::SliderFloat("##RestDensity", &m_restDensity,
                           500.0f, 2000.0f, "Rest density: %.0f"))
    {
        commands.hasSetRestDensity = true;
        commands.restDensityValue  = m_restDensity;
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Rest density of the fluid\nLower = sparse, higher = dense");

    ImGui::Spacing();

    if (ImGui::Checkbox("Artificial pressure", &m_artPressure))
    {
        commands.hasSetArtPressure  = true;
        commands.artPressureEnabled = m_artPressure;
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Reduces particle clustering\nat low density regions");

    ImGui::Spacing();
    ImGui::SetNextItemWidth(innerW);
    if (ImGui::SliderFloat("##Vorticity \xce\xb5", &m_vorticityEpsilon, 0.0f, 0.5f, "Vorticity: %.2f")) 
    {
        commands.hasSetVorticity = true;
        commands.vorticityEpsilon = m_vorticityEpsilon;
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Vorticity Confinement strength.\n"
                        "0 = off, ~0.05 typical.\n"
                        "Restores rotational detail lost by numerical damping.");
    
    ImGui::SetNextItemWidth(innerW);
    if (ImGui::SliderFloat("##XSPH Viscosity", &m_xsphViscosity, 0.0f, 0.6f, "Viscosity: %.2f")) 
    {
        commands.hasSetXSPH = true;
        commands.xsphViscosity = m_xsphViscosity;
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("XSPH velocity smoothing.\n0 = off, 0.02 typical.\n"
                          "Blends each particle's velocity toward its neighbours.\n"
                          "Makes the fluid look cohesive, removes jittering.");
}

//  Секция управления симуляцией
void SettingsPanel::drawSimSection(const AppState& state, AppCommands& commands)
{
    ImGui::TextColored(ImVec4(0.55f, 0.75f, 1.00f, 1.0f), "Simulation");
    ImGui::Spacing();

    bool pausedLocal = state.paused;
    if (ImGui::Checkbox("Paused", &pausedLocal))
    {
        commands.hasSetPaused   = true;
        commands.setPausedValue = pausedLocal;
    }

    ImGui::Spacing();

    const float btnW = (kPanelW - 28.0f) * 0.5f;
    if (ImGui::Button("Step once", ImVec2(btnW, 0)))
        commands.stepOnce = true;
    ImGui::SameLine();
    if (ImGui::Button("Reset (R)", ImVec2(btnW, 0)))
        commands.reset = true;
}

//  Секция взаимодействия мышью
void SettingsPanel::drawInteractionSection(const AppState& state, AppCommands& commands)
{
    ImGui::TextColored(ImVec4(0.55f, 0.75f, 1.00f, 1.0f), "Mouse Interaction");
    ImGui::Spacing();

    const float innerW = kPanelW - 20.0f;

    // Mode selector
    const char* modes[] = { "Force Application", "Container Rotation" };
    ImGui::SetNextItemWidth(innerW);
    if (ImGui::Combo("##InteractionMode", &m_interactionMode, modes, 2)) {
        commands.hasSetInteractionMode = true;
        commands.interactionMode = m_interactionMode;
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Force: Apply forces with mouse\nRotation: Rotate container (stub)");

    ImGui::Spacing();

    // Radius slider (only enabled in Force mode)
    ImGui::BeginDisabled(m_interactionMode != 0);
    ImGui::SetNextItemWidth(innerW);
    if (ImGui::SliderFloat("##ForceRadius", &m_mouseForceRadius, 0.5f, 3.0f, "Radius: %.2f")) {
        commands.hasSetMouseForceRadius = true;
        commands.mouseForceRadius = m_mouseForceRadius;
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Mouse force application radius\nAdjust with scroll wheel");
    ImGui::EndDisabled();
}

//  Секция камеры
void SettingsPanel::drawCameraSection(const Camera3D& camera, AppCommands& commands)
{
    const float innerW = kPanelW - 20.0f;

    ImGui::TextColored(ImVec4(0.55f, 0.75f, 1.00f, 1.0f), "Camera");
    ImGui::Spacing();

    const auto eye = camera.getEye();
    ImGui::Text("Eye: (%.1f, %.1f, %.1f)", eye.x, eye.y, eye.z);

    ImGui::Text("Yaw: %.0f°  Pitch: %.0f°",
        camera.getYaw(), camera.getPitch());

    ImGui::Spacing();

    const float btnW = kPanelW - 28.0f;
    if (ImGui::Button("Reset View", ImVec2(btnW, 0)))
        commands.resetCamera = true;
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Reset camera to default position");
}