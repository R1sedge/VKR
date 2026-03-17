#pragma once

#include "simCUDA/cudaParticles.cuh"
#include "deviceNeighborList.cuh"

void buildNeighborsNaiveCUDA(
    const DeviceParticles2D& particles,
    DeviceNeighborList& nl,
    float smoothingRadius);