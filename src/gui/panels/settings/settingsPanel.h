#pragma once
#include <imgui.h>

struct AppState;
struct AppCommands;

//   Боковая панель настроек, выезжающая слева при приближении курсора.
//   Остаётся открытой пока активен любой ImGui-элемент (слайдер, кнопка).
class SettingsPanel
{
public:
    void draw(const AppState& state, AppCommands& commands);

    void setRestDensity       (float rd)     { m_restDensity = rd; }
    void setArtPressureEnabled(bool enabled) { m_artPressure = enabled; }

private:
    float m_anim        = 0.0f;
    float m_restDensity = 200.0f;
    bool  m_artPressure = true;

    static constexpr float kPanelW    = 260.0f;
    static constexpr float kTriggerX  = 96.0f;
    static constexpr float kAnimSpeed = 10.0f;

    void drawEdgeHint  (const ImGuiIO& io);
    void drawPbfSection(AppCommands& commands);
    void drawSimSection(const AppState& state, AppCommands& commands);
};