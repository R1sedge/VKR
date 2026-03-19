#pragma once
#include "simCUDA/utils/cudaParticles.cuh"

struct DeviceNeighborList {
    int  particleCount = 0;
    int  idsCount      = 0;
    int  idsCapacity   = 0;
    int* counts        = nullptr;  // [n]
    int* offsets       = nullptr;  // [n+1]
    int* ids           = nullptr;  // [idsCount]
};

void allocateDeviceNeighborList(DeviceNeighborList& nl, int n);
void freeDeviceNeighborList(DeviceNeighborList& nl);