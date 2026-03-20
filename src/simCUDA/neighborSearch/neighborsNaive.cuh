#pragma once

#include "simCUDA/utils/cudaParticles.cuh"
#include "simCUDA/neighborSearch/deviceNeighborList.cuh"

void buildNeighborsNaiveCUDA(
    const DeviceParticles2D& particles,
    DeviceNeighborList& nl,
    float smoothingRadius);