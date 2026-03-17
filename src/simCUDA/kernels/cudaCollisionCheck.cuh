#pragma once

#include "simCUDA/cudaParticles.cuh"
#include "simCUDA/neighborSearch/deviceNeighborList.cuh"

struct DeviceCollisionCheck
{
    int particleCount = 0;

    int* particleFlags = nullptr; 
    int* particleCounts = nullptr;  
    int* pairCount = nullptr;       
};

void allocateDeviceCollisionCheck(DeviceCollisionCheck& cc, int particleCount);
void freeDeviceCollisionCheck(DeviceCollisionCheck& cc);
void resetDeviceCollisionCheck(DeviceCollisionCheck& cc);

void launchCheckParticleCollisions(
    const DeviceParticles2D& particles,
    const DeviceNeighborList& neighbors,
    DeviceCollisionCheck& cc,
    float particleRadius);
