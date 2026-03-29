#pragma once
#include "common/Config.h"

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

    void clear() {*this = AppCommands();}
};