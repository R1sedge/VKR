#pragma once

#include "simCUDA/utils/cudaParticles.cuh"
#include "simCUDA/neighborSearch/deviceNeighborList.cuh"

void launchComputeDeltaPositions(
    const DeviceParticles3D& particles,
    const DeviceNeighborList& neighbors,
    float restDensity,
    float smoothingRadius,
    float artPressureK,
    float wDeltaQ
);

void launchApplyDeltaPositions(DeviceParticles3D& particles, float scale = 0.001f);