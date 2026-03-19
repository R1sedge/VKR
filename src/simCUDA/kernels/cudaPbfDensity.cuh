#pragma once

#include "simCUDA/neighborSearch/deviceNeighborList.cuh"
#include "simCUDA/utils/cudaParticles.cuh"

void launchComputeDensity(
    const DeviceParticles2D& particles,
    const DeviceNeighborList& neighbors,
    float smoothingRadius
);