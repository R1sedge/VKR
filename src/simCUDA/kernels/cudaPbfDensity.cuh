#pragma once

#include "simCUDA/neighborSearch/deviceNeighborList.cuh"
#include "simCUDA/cudaParticles.cuh"

void launchComputeDensity(
    const DeviceParticles2D& particles,
    const DeviceNeighborList& neighbors,
    float smoothingRadius
);