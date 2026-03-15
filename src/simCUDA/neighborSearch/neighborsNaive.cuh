#pragma once

#include "simCUDA/cudaParticles.cuh"

struct DeviceNeighborList
{
    int particleCount = 0;
    int idsCount = 0;
    int idsCapacity = 0;

    int* counts = nullptr;  // n
    int* offsets = nullptr; // n + 1
    int* ids = nullptr;     // idsCount
};

void allocateDeviceNeighbotList(DeviceNeighborList& nl, int particleCount);
void freeDeviceNeighborList(DeviceNeighborList& nl);

void buildNeighborsNaiveCUDA(
    const DeviceParticles2D& particles,
    DeviceNeighborList& nl,
    float smoothingRadius);