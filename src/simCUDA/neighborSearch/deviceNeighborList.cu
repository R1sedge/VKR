#include "simCUDA/neighborSearch/deviceNeighborList.cuh"
#include "simCUDA/utils/cudaCheck.h"
#include "simCUDA/utils/cudaMemUtils.cuh"

#include <cuda_runtime.h>

void allocateDeviceNeighborList(DeviceNeighborList& nl, int n)
{
    freeDeviceNeighborList(nl);
    nl.particleCount = n;
    if (n > 0)
    { 
        CudaMem::allocIntArray(nl.counts, n + 1); 
        CudaMem::allocIntArray(nl.offsets, n + 1); 
    }
}

void freeDeviceNeighborList(DeviceNeighborList& nl)
{
    CudaMem::freeIntArray(nl.counts); 
    CudaMem::freeIntArray(nl.offsets); 
    CudaMem::freeIntArray(nl.ids);

    nl.particleCount = 0;
    nl.idsCount = 0;
    nl.idsCapacity = 0;
}
