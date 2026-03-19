#include "simCUDA/kernels/cudaCollisionCheck.cuh"

#include <cuda_runtime.h>

#include "simCUDA/utils/cudaCheck.h"
#include "simCUDA/utils/cudaUtils.cuh"
#include "simCUDA/utils/cudaMemUtils.cuh"

namespace
{
    __global__ void checkParticleCollisionsKernel(
        int n,
        const float* x,
        const float* y,
        const int* neighborOffsets,
        const int* neighborIds,
        int* particleFlags,
        int* particleCounts,
        int* pairCount,
        float minDist2)
    {
        const int i = blockIdx.x * blockDim.x + threadIdx.x;
        if (i >= n) return;

        const float xi = x[i];
        const float yi = y[i];

        int localCount = 0;

        const int begin = neighborOffsets[i];
        const int end   = neighborOffsets[i + 1];

        for (int k = begin; k < end; ++k)
        {
            const int j = neighborIds[k];
            if (j <= i) continue;

            const float dx = xi - x[j];
            const float dy = yi - y[j];
            const float dist2 = dx * dx + dy * dy;

            if (dist2 < minDist2)
            {
                ++localCount;

                atomicExch(&particleFlags[i], 1);
                atomicExch(&particleFlags[j], 1);

                atomicAdd(&particleCounts[j], 1);
                atomicAdd(pairCount, 1);
            }
        }

        if (localCount > 0)
            atomicAdd(&particleCounts[i], localCount);
    }
}

void allocateDeviceCollisionCheck(DeviceCollisionCheck& cc, int particleCount)
{
    freeDeviceCollisionCheck(cc);

    cc.particleCount = particleCount;
    if (particleCount <= 0) return;

    CudaMem::allocIntArray(cc.particleFlags, particleCount);
    CudaMem::allocIntArray(cc.particleCounts, particleCount);
    CudaMem::allocIntArray(cc.pairCount, 1);
}

void freeDeviceCollisionCheck(DeviceCollisionCheck& cc)
{
    CudaMem::freeIntArray(cc.particleFlags);
    CudaMem::freeIntArray(cc.particleCounts);
    CudaMem::freeIntArray(cc.pairCount);

    cc.particleCount = 0;
}

void resetDeviceCollisionCheck(DeviceCollisionCheck& cc)
{
    if (cc.particleCount <= 0) return;

    CUDA_CHECK(cudaMemset(cc.particleFlags, 0, sizeof(int) * cc.particleCount));
    CUDA_CHECK(cudaMemset(cc.particleCounts, 0, sizeof(int) * cc.particleCount));
    CUDA_CHECK(cudaMemset(cc.pairCount, 0, sizeof(int)));
}

void launchCheckParticleCollisions(
    const DeviceParticles2D& particles,
    const DeviceNeighborList& neighbors,
    DeviceCollisionCheck& cc,
    float particleRadius)
{
    const int n = particles.count;
    if (n <= 0) return;

    if (cc.particleCount != n)
        allocateDeviceCollisionCheck(cc, n);
    
    resetDeviceCollisionCheck(cc);

    const float minDist = 2.0f * particleRadius;
    const float minDist2 = minDist * minDist;

    checkParticleCollisionsKernel<<<CudaUtils::gridSize(n), CudaUtils::BLOCK_SIZE>>>(
        n,
        particles.x,
        particles.y,
        neighbors.offsets,
        neighbors.ids,
        cc.particleFlags,
        cc.particleCounts,
        cc.pairCount,
        minDist2);

    CUDA_CHECK(cudaGetLastError());
}
