#include "simCUDA/neighborSearch/neighborsNaive.cuh"

#include <cuda_runtime.h>
#include <vector>

#include "simCUDA/utils/cudaCheck.h"
#include "simCUDA/utils/cudaUtils.cuh"
#include "simCUDA/utils/cudaMemUtils.cuh"

namespace
{
    void ensureIdsCapacity(DeviceNeighborList& nl, int required)
    {
        if (required <= nl.idsCapacity)
        {
            return;
        }

        CudaMem::freeIntArray(nl.ids);
        nl.idsCapacity = required;
        nl.idsCount = required;

        if (required > 0)
            CudaMem::allocIntArray(nl.ids, required);
    }

    __global__ void countNeighborsKernel(
        int n,
        const float* x,
        const float* y,
        float h2,
        int* counts)
    {
        const int i = blockIdx.x * blockDim.x + threadIdx.x;
        if (i >= n) return;

        const float xi = x[i];
        const float yi = y[i];

        int count = 0;
        for (int j = 0; j < n; ++j)
        {
            if (j == i) continue;

            const float dx = xi  - x[j];
            const float dy = yi - y [j];

            if (dx * dx + dy * dy < h2)
                ++count;
        }

        counts[i] = count;
    }

    __global__ void fillNeighborsKernel(
        int n,
        const float* x,
        const float* y,
        float h2,
        const int* offsets,
        int* ids)
    {
        const int i = blockIdx.x * blockDim.x + threadIdx.x;
        if (i >= n) return;

        float xi = x[i];
        float yi = y[i];

        int write = offsets[i];
        for (int j = 0; j < n; ++j)
        {
            if (j == i) continue;

            float dx = xi - x[j];
            float dy = yi - y[j];

            if (dx * dx + dy * dy < h2)
                ids[write++] = j;
        }
    }
}

void buildNeighborsNaiveCUDA(
    const DeviceParticles3D& particles,
    DeviceNeighborList& nl,
    float smoothingRadius)
{
    const int n = particles.count;

    if (nl.particleCount != n)
        allocateDeviceNeighborList(nl, n);
    
    if (n <= 0)
    {
        nl.idsCount = 0;
        return;
    }

    const float h2 = smoothingRadius * smoothingRadius;

    countNeighborsKernel<<<CudaUtils::gridSize(n), CudaUtils::BLOCK_SIZE>>>(
        n,
        particles.x,
        particles.y,
        h2,
        nl.counts);
        
    CUDA_CHECK(cudaGetLastError());
    CUDA_CHECK(cudaDeviceSynchronize());
    
    std::vector<int> hostCounts(n);
    CUDA_CHECK(cudaMemcpy(hostCounts.data(), nl.counts, sizeof(int) * n, cudaMemcpyDeviceToHost));

    std::vector<int> hostOffsets(n + 1, 0);
    int totalIds = 0;
    for (int i = 0; i < n; ++i)
    {
        hostOffsets[i] = totalIds;
        totalIds += hostCounts[i];
    }
    hostOffsets[n] = totalIds;

    ensureIdsCapacity(nl, totalIds);

    CUDA_CHECK(cudaMemcpy(
        nl.offsets,
        hostOffsets.data(),
        sizeof(int) * (n + 1),
        cudaMemcpyHostToDevice));

    if (totalIds <= 0)
        return;

    fillNeighborsKernel<<<CudaUtils::gridSize(n), CudaUtils::BLOCK_SIZE>>>(
        n,
        particles.x,
        particles.y,
        h2,
        nl.offsets,
        nl.ids);
    CUDA_CHECK(cudaGetLastError());
}