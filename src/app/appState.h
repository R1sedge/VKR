#pragma once

#include "sim/simulationBackendType.h"

struct AppState
{
    bool paused = true;
    bool artPressureEnabled = true;
    int activeSceneIndex = 0;

    SimulationBackendType backendType = SimulationBackendType::CUDA;
};