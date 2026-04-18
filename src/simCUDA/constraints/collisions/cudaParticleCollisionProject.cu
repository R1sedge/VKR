#include "simCUDA/constraints/collisions/cudaParticleCollisionProject.cuh"

#include <cuda_runtime.h>
#include <math.h>

#include "simCUDA/utils/cudaCheck.h"
#include "simCUDA/utils/cudaUtils.cuh"

namespace
{
    __global__ void accumulateParticleCollisionDeltaKernel(
        int n,
        const float* x,
        const float* y,
        const float* z,
        const float* mass,
        const int* neighborOffsets,
        const int* neighborIds,
        float* dxOut,
        float* dyOut,
        float* dzOut,
        float minDist,
        float eps)
    {
        const int i = blockIdx.x * blockDim.x + threadIdx.x;
        if (i >= n) return;

        const float xi = x[i];
        const float yi = y[i];
        const float zi = z[i];
        const float mi = mass[i];

        const int begin = neighborOffsets[i];
        const int end   = neighborOffsets[i + 1];

        for (int k = begin; k < end; ++k)
        {
            const int j = neighborIds[k];
            if (j <= i) continue;

            const float dx = xi - x[j];
            const float dy = yi - y[j];
            const float dz = zi - z[j];

            float dist = sqrtf(dx * dx + dy * dy + dz * dz);
            if (dist < eps) dist = eps;

            const float overlap = minDist - dist;
            if (overlap <= 0.0f) continue;

            const float invDist = 1.0f / dist;
            const float nx = dx * invDist;
            const float ny = dy * invDist;
            const float nz = dz * invDist;

            const float mj = mass[j];
            const float invMassSum = 1.0f / (mi + mj);
            const float wi = mj * invMassSum;
            const float wj = mi * invMassSum;

            const float corrX = overlap * nx;
            const float corrY = overlap * ny;
            const float corrZ = overlap * nz;

            atomicAdd(&dxOut[i],  wi * corrX);
            atomicAdd(&dyOut[i],  wi * corrY);
            atomicAdd(&dzOut[i],  wi * corrZ);
            atomicAdd(&dxOut[j], -wj * corrX);
            atomicAdd(&dyOut[j], -wj * corrY);
            atomicAdd(&dzOut[j], -wj * corrZ);
        }
    }

    __global__ void applyParticleCollisionDeltaKernel(
        int n,
        float* x,
        float* y,
        float* z,
        float* dx,
        float* dy,
        float* dz)
    {
        const int i = blockIdx.x * blockDim.x + threadIdx.x;
        if (i >= n) return;

        x[i] += dx[i];
        y[i] += dy[i];
        z[i] += dz[i];

        dx[i] = 0.0f;
        dy[i] = 0.0f;
        dz[i] = 0.0f;
    }
}

void launchProjectParticleCollisions(
    DeviceParticles3D& particles,
    const DeviceNeighborList& neighbors,
    float particleRadius)
{
    if (particles.count <= 0)
        return;

    const float minDist = 2.0f * particleRadius;
    const float eps = 1e-6f;

    accumulateParticleCollisionDeltaKernel<<<CudaUtils::gridSize(particles.count), CudaUtils::BLOCK_SIZE>>>(
        particles.count,
        particles.x,
        particles.y,
        particles.z,
        particles.mass,
        neighbors.offsets,
        neighbors.ids,
        particles.dx,
        particles.dy,
        particles.dz,
        minDist,
        eps);
    CUDA_CHECK(cudaGetLastError());

    applyParticleCollisionDeltaKernel<<<CudaUtils::gridSize(particles.count), CudaUtils::BLOCK_SIZE>>>(
        particles.count,
        particles.x,
        particles.y,
        particles.z,
        particles.dx,
        particles.dy,
        particles.dz);
    CUDA_CHECK(cudaGetLastError());
}
