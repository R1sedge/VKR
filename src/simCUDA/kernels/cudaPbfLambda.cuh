#pragma once

#include "simCUDA/cudaParticles.cuh"
#include "simCUDA/neighborSearch/neighborsNaive.cuh"

void launchComputeLambda(
    const DeviceParticles2D& particles,
    const DeviceNeighborList& neighbors,
    float restDensity,
    float epsilon,
    float smoothingRadius);
