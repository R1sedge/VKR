#pragma once

#include <imgui.h>

#include "common/Config.h"
#include "sim/simulationBackendType.h"

class Camera3D;
struct AppState;
struct AppCommands;

class SettingsPanel
{
public:
    void draw(const AppState& state, const Camera3D& camera, AppCommands& commands);

    void setRestDensity(float rd) { m_restDensity = rd; }
    void setArtPressureEnabled(bool enabled) { m_artPressure = enabled; }
    void setVorticityEpsilon(float val) { m_vorticityEpsilon = val; }
    void setXsphViscosity(float val) { m_xsphViscosity = val; }

    void setBackendType(SimulationBackendType type) { m_backendType = type; }

    void setGravity(float x, float y, float z)
    {
        m_gravity[0] = x;
        m_gravity[1] = y;
        m_gravity[2] = z;
    }

    void setMaxSpeed(float val) { m_maxSpeed = val; }
    void setWallResponse(float r, float f) { m_wallRestitution = r; m_wallFriction = f; }
    void setArtificialPressureK(float val) { m_artificialPressureK = val; }

private:
    float m_anim = 0.0f;
    static constexpr float kPanelW = 260.0f;
    static constexpr float kTriggerX = 96.0f;
    static constexpr float kAnimSpeed = 10.0f;
    static constexpr float kHintW = 48.0f;
    static constexpr float kHintH = 90.0f;

    // Backend
        SimulationBackendType m_backendType = SimulationBackendType::CUDA;

    // PBF
    float m_restDensity = Config::restDensity;
    bool m_artPressure = true;
    float m_vorticityEpsilon = Config::vorticityEpsilon;
    float m_xsphViscosity = Config::xsphViscosity;
    float m_artificialPressureK = Config::artificialPressureK;

    // Runtime
    float m_gravity[3] = { Config::gravityX, Config::gravityY, Config::gravityZ };
    float m_maxSpeed = Config::maxSpeed;
    float m_wallRestitution = Config::wallRestitution;
    float m_wallFriction = Config::wallFriction;
    bool  m_baffleFiltering = Config::enableBafflePairFiltering;

    // Отрисовка
    int m_particleColorMode = Config::particleColorMode;
    float m_maxGradSpeed = Config::maxGradSpeed;
    float m_phase0Color[4] = {
        Config::phase0Color.r,
        Config::phase0Color.g,
        Config::phase0Color.b,
        Config::phase0Color.a
    };
    float m_phase1Color[4] = {
        Config::phase1Color.r,
        Config::phase1Color.g,
        Config::phase1Color.b,
        Config::phase1Color.a
    };

    void drawEdgeHint(const ImGuiIO& io, bool mouseNear);
    void drawSectionHeader(const char* label);

    void drawPbfSection(AppCommands& commands);
    void drawRuntimeSection(AppCommands& commands);
    void drawRenderSection(AppCommands& commands);
    void drawSimSection(const AppState& state, AppCommands& commands);
    void drawCameraSection(const Camera3D& camera, AppCommands& commands);
    void drawControlsSection();
    void drawConstantsSection();
};