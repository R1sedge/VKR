#include "simCUDA/neighborSearch/deviceNeighborList.cuh"
#include "simCUDA/cudaCheck.h"

#include <cuda_runtime.h>

namespace 
{
    void allocIntArray(int*& ptr, int count)
    {
        CUDA_CHECK(cudaMalloc(&ptr, sizeof(int) * count));
    }

    void freeIntArray(int*& ptr)
    {
        if (ptr != nullptr)
        {
            CUDA_CHECK(cudaFree(ptr));
            ptr = nullptr;
        }
    }
}

void allocateDeviceNeighborList(DeviceNeighborList& nl, int n)
{
    freeDeviceNeighborList(nl);
    nl.particleCount = n;
    if (n > 0)
    { 
        allocIntArray(nl.counts, n); 
        allocIntArray(nl.offsets, n + 1); 
    }
}

void freeDeviceNeighborList(DeviceNeighborList& nl)
{
    freeIntArray(nl.counts); 
    freeIntArray(nl.offsets); 
    freeIntArray(nl.ids);

    nl.particleCount = 0;
    nl.idsCount = 0;
    nl.idsCapacity = 0;
}
