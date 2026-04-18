#pragma once
#include "simCUDA/utils/cudaParticles.cuh"

void launchApplyMouseForce(
    DeviceParticles3D& particles,
    float mouseX, float mouseY,
    float radius, float strength,
    int forceType);
