#pragma once
#include <imgui.h>
#include "common/Config.h"

struct AppState;
struct AppCommands;

//   Боковая панель настроек, выезжающая слева при приближении курсора.
//   Остаётся открытой пока активен любой ImGui-элемент (слайдер, кнопка).
class SettingsPanel
{
    public:
        void draw(const AppState& state, AppCommands& commands);

        void setRestDensity(float rd) { m_restDensity = rd; }
        void setArtPressureEnabled(bool enabled) { m_artPressure = enabled; }
        void setVorticityEpsilon(float val) { m_vorticityEpsilon = val; }
        void setXsphViscosity(float val) { m_xsphViscosity = val; }
        
    private:
        float m_anim = 0.0f;
        float m_restDensity = Config::restDensity;
        bool  m_artPressure = true;

        static constexpr float kPanelW = 260.0f;
        static constexpr float kTriggerX = 96.0f;
        static constexpr float kAnimSpeed = 10.0f;
        static constexpr float kHintW = 48.0f;
        static constexpr float kHintH = 90.0f;

        float m_vorticityEpsilon = Config::vorticityEpsilon;
        float m_xsphViscosity = Config::xsphViscosity;

        void drawEdgeHint(const ImGuiIO& io, bool mouseNear);
        void drawPbfSection(AppCommands& commands);
        void drawSimSection(const AppState& state, AppCommands& commands);
};