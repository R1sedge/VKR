#pragma once

#include "simCUDA/utils/cudaParticles.cuh"
#include "simCUDA/neighborSearch/deviceNeighborList.cuh"

void launchProjectParticleCollisions(
    DeviceParticles3D& particles,
    const DeviceNeighborList& neighbors,
    float particleRadius);