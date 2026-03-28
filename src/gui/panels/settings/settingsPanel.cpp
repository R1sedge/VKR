#include "settingsPanel.h"
#include "app/appState.h"
#include "app/appCommands.h"

#include <imgui.h>
#include <cmath>
#include <algorithm>

void SettingsPanel::draw(const AppState& state, AppCommands& commands)
{
    const ImGuiIO& io = ImGui::GetIO();
    const float    dt = io.DeltaTime;

    // Держим панель открытой если:
    //  - курсор в зоне триггера у левого края, ИЛИ
    //  - ImGui сейчас обрабатывает активный элемент (слайдер захвачен, кнопка зажата)
    const float visibleEdge = kPanelW * m_anim;  // реальный правый край панели
    const float triggerEdge = std::max(kTriggerX, visibleEdge);
    const bool mouseNear    = (io.MousePos.x >= 0.0f && io.MousePos.x < triggerEdge);

    const bool itemActive = ImGui::IsAnyItemActive();
    const float target    = (mouseNear || itemActive) ? 1.0f : 0.0f;

    m_anim += (target - m_anim) * (1.0f - std::exp(-kAnimSpeed * dt));

    // Рисуем хинт у левого края когда панель скрыта
    if (m_anim < 0.95f)
        drawEdgeHint(io);

    if (m_anim < 0.002f) return;

    // Позиция: выезжает из-за левого края
    const float xPos   = -kPanelW + kPanelW * m_anim;
    const float panelH = io.DisplaySize.y;

    ImGui::SetNextWindowPos (ImVec2(xPos, 0.0f),      ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(kPanelW, panelH), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.88f);

    ImGui::Begin("##SettingsPanel", nullptr,
        ImGuiWindowFlags_NoDecoration    |
        ImGuiWindowFlags_NoMove          |
        ImGuiWindowFlags_NoResize        |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoFocusOnAppearing
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
    ImGui::End();
}

//  Хинт у левого края (виден когда панель убрана)
void SettingsPanel::drawEdgeHint(const ImGuiIO& io) //TODO Нужно добавить иконку
{
    // Прозрачность хинта: максимальна когда панель полностью убрана
    const float alpha = (1.0f - m_anim) * 1.0f;
    if (alpha < 0.01f) return;

    const float h       = 22.0f;
    const float centerY = io.DisplaySize.y * 0.5f;

    ImGui::SetNextWindowPos (ImVec2(0.0f, centerY - h * 0.5f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(14.0f, h),                  ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(alpha);

    ImGui::Begin("##SettingsHint", nullptr,
        ImGuiWindowFlags_NoDecoration    |
        ImGuiWindowFlags_NoInputs        |
        ImGuiWindowFlags_NoNav           |
        ImGuiWindowFlags_NoMove          |
        ImGuiWindowFlags_NoSavedSettings
    );

    // Маленькая стрелка вправо как индикатор
    ImGui::TextColored(ImVec4(0.70f, 0.85f, 1.00f, alpha / 0.55f), ">");

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
                           30.0f, 1000.0f, "Rest density: %.0f"))
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