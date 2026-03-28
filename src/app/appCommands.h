#pragma once
#include "common/Config.h"

struct AppCommands
{
    bool togglePause = false;

    bool hasSetPaused = false;
    bool setPausedValue = false;

    bool stepOnce = false;
    bool reset = false;

    bool hasSetRestDensity = false;
    float restDensityValue = Config::restDensity;

    bool hasSetArtPressure  = false;
    bool artPressureEnabled = true;

    bool  hasSetVorticity = false;
    float vorticityEpsilon = Config::vorticityEpsilon;

    bool hasSetXSPH = false;
    float xsphViscosity = Config::xsphViscosity;

    void clear() {*this = AppCommands();}
};