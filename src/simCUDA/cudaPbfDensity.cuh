#pragma once

#include "simCUDA/neighborSearch/neighborsNaive.cuh"
#include "simCUDA/cudaParticles.cuh"

void launchComputeDensity(
    const DeviceParticles2D& particles,
    const DeviceNeighborList& neighbors,
    float smoothingRadius
);