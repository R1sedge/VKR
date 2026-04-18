#pragma once

#include "simCUDA/utils/cudaParticles.cuh"
#include "simCUDA/neighborSearch/deviceNeighborList.cuh"

void launchComputeLambda(
    const DeviceParticles3D& particles,
    const DeviceNeighborList& neighbors,
    float restDensity,
    float epsilon,
    float smoothingRadius);
