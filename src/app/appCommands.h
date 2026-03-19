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

    void clear() {*this = AppCommands();}
};