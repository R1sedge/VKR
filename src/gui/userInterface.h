#pragma once
#include <GLFW/glfw3.h>

#include "gui/timing/frameTimer.h"
#include "gui/panels/fps/fpsOverlay.h"
#include "gui/panels/stats/statsPanel.h"
#include "gui/panels/settings/settingsPanel.h"
#include "gui/panels/scene/scenePanel.h"

class Camera3D;
struct AppState;
struct AppCommands;

class UserInterface
{
public:
    UserInterface() = default;
    ~UserInterface();

    bool initialize(GLFWwindow* window);

    void beginFrame();
    void buildUI(const AppState& state, const Camera3D& camera, AppCommands& commands);
    void endFrame();

    void setFrameTiming(double frameTimeSeconds);
    void setPhysicsTiming(double seconds);
    void setRenderTiming(double seconds);

    void setSimulationDt(float dt) { m_simDt = dt; }
    void setParticleCount(int count) { m_fps.setParticleCount(count); }

    void setRestDensity(float rd) { m_settings.setRestDensity(rd); }
    void setArtPressure(bool enabled) { m_settings.setArtPressureEnabled(enabled); }
    void setVorticityEpsilon(float val) { m_settings.setVorticityEpsilon(val); }
    void setXsphViscosity(float val) { m_settings.setXsphViscosity(val); }

    void setSceneIndex(int idx) { m_scenes.setSceneIndex(idx); }
    void setInteractionMode(int mode) { m_settings.setInteractionMode(mode); }
    void setMouseForceRadius(float radius) { m_settings.setMouseForceRadius(radius); }

private:
    bool initialized = false;

    float m_simDt = 0.0f;

    FrameTimer m_timer;
    FpsOverlay m_fps;
    StatsPanel m_stats;
    SettingsPanel m_settings;
    ScenePanel m_scenes;
};