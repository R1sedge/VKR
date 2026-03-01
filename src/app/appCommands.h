#pragma once

struct AppCommands
{
    bool togglePause = false;

    bool hasSetPaused = false;
    bool setPausedValue = false;

    bool stepOnce = false;
    bool reset = false;

    void clear() {*this = AppCommands();}
};