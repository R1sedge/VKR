#pragma once

#include "simCUDA/cudaParticles.cuh"

void launchProjectBounds(
    DeviceParticles2D& dp,
    float left,
    float right,
    float bottom,
    float top,
    float radius);
