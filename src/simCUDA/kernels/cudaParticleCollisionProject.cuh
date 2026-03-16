#pragma once

#include "simCUDA/cudaParticles.cuh"
#include "simCUDA/neighborSearch/neighborsNaive.cuh"

void launchProjectParticleCollisions(
    DeviceParticles2D& particles,
    const DeviceNeighborList& neighbors,
    float particleRadius);