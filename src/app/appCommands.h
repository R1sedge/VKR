#pragma once

struct AppCommands
{
    bool togglePause = false;

    bool hasSetPaused = false;
    bool setPausedValue = false;

    bool stepOnce = false;
    bool reset = false;

    bool hasSetRestDensity = false;
    float restDensityValue = 200.0f;

    bool hasSetArtPressure  = false;
    bool artPressureEnabled = true;

    void clear() {*this = AppCommands();}
};