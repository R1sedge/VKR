#pragma once

#include "simCUDA/cudaParticles.cuh"
#include "simCUDA/neighborSearch/neighborsNaive.cuh"

void launchComputeDeltaPositions(
    const DeviceParticles2D& particles,
    const DeviceNeighborList& neighbors,
    float restDensity,
    float smoothingRadius);

void launchApplyDeltaPositions(
    DeviceParticles2D& particles,
    float scale = 0.001f);