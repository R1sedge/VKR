#pragma once
#include "common/Config.h"
#include "app/appState.h"

struct AppCommands
{
    // Управление
    bool togglePause = false;

    bool hasSetPaused = false;
    bool setPausedValue = false;

    bool stepOnce = false;
    bool reset = false;

    // PBF
    bool hasSetRestDensity = false;
    float restDensityValue = Config::restDensity;

    bool hasSetArtPressure  = false;
    bool artPressureEnabled = true;

    bool hasSetVorticity = false;
    float vorticityEpsilon = Config::vorticityEpsilon;

    bool hasSetXSPH = false;
    float xsphViscosity = Config::xsphViscosity;

    // Сцена
    bool hasSetScene = false;
    int sceneIndex = 0;

    // Mouse interaction
    bool hasMouseForce = false;
    float mouseForceWorldX = 0.0f;
    float mouseForceWorldY = 0.0f;
    float mouseForceRadius = 1.0f;
    float mouseForceStrength = 1.0f;
    int mouseForceType = 0;  // 0=repulsion, 1=attraction, 2=vortex

    // Interaction mode
    bool hasSetInteractionMode = false;
    int interactionMode = InteractionModeCameraControl;  // 0=camera, 1=vessel, 2=force

    // Mouse force radius update
    bool hasSetMouseForceRadius = false;

    // Camera
    bool resetCamera = false;

    void clear() {*this = AppCommands();}
};