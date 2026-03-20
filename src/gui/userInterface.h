#pragma once
#include <GLFW/glfw3.h>

#include "gui/timing/frameTimer.h"
#include "gui/panels/fps/fpsOverlay.h"
#include "gui/panels/stats/statsPanel.h"
#include "gui/panels/settings/settingsPanel.h"

struct AppState;
struct AppCommands;


//   Тонкий оркестратор: инициализирует ImGui, владеет панелями,
//   делегирует отрисовку каждой панели своему классу.
class UserInterface
{
public:
    UserInterface() = default;
    ~UserInterface();

    bool initialize(GLFWwindow* window);

    void beginFrame();
    void buildUI(const AppState& state, AppCommands& commands);
    void endFrame();

    // Передача метрик из app.cpp
    void setFrameTiming  (double frameTimeSeconds);
    void setPhysicsTiming(double seconds);
    void setRenderTiming (double seconds);

    void setSimulationDt  (float dt)      { m_simDt = dt; }
    void setRestDensity   (float rd)      { m_settings.setRestDensity(rd); }
    void setArtPressure   (bool enabled)  { m_settings.setArtPressureEnabled(enabled); }

private:
    bool initialized = false;

    float m_simDt = 0.0f;

    FrameTimer   m_timer;
    FpsOverlay   m_fps;
    StatsPanel   m_stats;
    SettingsPanel m_settings;
};