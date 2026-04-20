#include "simCUDA/pbf/cudaPbfDeltaPositions.cuh"

#include <cuda_runtime.h>
#include <math.h>

#include "simCUDA/utils/cudaCheck.h"
#include "simCUDA/utils/cudaUtils.cuh"
#include "simCUDA/utils/cudaSphKernels.cuh"

namespace
{
    __global__ void computeDeltaPositionsKernel(
        int n,
        const float* __restrict__ x,
        const float* __restrict__ y,
        const float* __restrict__ z,
        const float* __restrict__ mass,
        const float* __restrict__ lambda,
        const int*   __restrict__ neighborOffsets,
        const int*   __restrict__ neighborIds,
        float* dxOut,
        float* dyOut,
        float* dzOut,
        float restDensity,
        float h,
        float artPressureK,
        float wDeltaQ)
    {
        const int i = blockIdx.x * blockDim.x + threadIdx.x;
        if (i >= n) return;

        const float invRestDensity = 1.0f / restDensity;
        const float gradEps = 1e-6f;

        const float xi = x[i];
        const float yi = y[i];
        const float zi = z[i];
        const float lambdaI = lambda[i];

        float deltaX = 0.0f;
        float deltaY = 0.0f;
        float deltaZ = 0.0f;

        const int begin = neighborOffsets[i];
        const int end   = neighborOffsets[i + 1];

        for (int k = begin; k < end; ++k)
        {
            const int j = neighborIds[k];

            const float dx = xi - x[j];
            const float dy = yi - y[j];
            const float dz = zi - z[j];
            const float r2 = dx * dx + dy * dy + dz * dz;

            if (r2 < gradEps) continue;

            const float r = sqrtf(r2);
            const float invR = 1.0f / r;

            // scorr = -k * (W(r,h) / W(deltaQ*h, h))^4
            // Если wDeltaQ == 0 (artPressureK == 0), scorr тоже будет 0
            const float wij   = CudaSPH::poly6(r, h);
            const float ratio = (wDeltaQ > 1e-30f) ? (wij / wDeltaQ) : 0.0f;
            const float ratio2 = ratio * ratio;
            const float scorr  = -artPressureK * (ratio2 * ratio2);

            const float gradW = CudaSPH::spikyGradCoeff(r, h);
            const float coeff = (lambdaI + lambda[j] + scorr) * mass[j] * invRestDensity * gradW;

            deltaX += coeff * dx * invR;
            deltaY += coeff * dy * invR;
            deltaZ += coeff * dz * invR;
        }

        dxOut[i] = deltaX;
        dyOut[i] = deltaY;
        dzOut[i] = deltaZ;
    }

    __global__ void applyDeltaPositionsKernel(
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

void launchComputeDeltaPositions(
    const DeviceParticles3D& particles,
    const DeviceNeighborList& neighbors,
    float restDensity,
    float smoothingRadius,
    float artPressureK,
    float wDeltaQ)
{
    if (particles.count <= 0) return;

    computeDeltaPositionsKernel<<<CudaUtils::gridSize(particles.count), CudaUtils::BLOCK_SIZE>>>(
        particles.count,
        particles.x, particles.y, particles.z,
        particles.mass,
        particles.lambda,
        neighbors.offsets, neighbors.ids,
        particles.dx, particles.dy, particles.dz,
        restDensity,
        smoothingRadius,
        artPressureK, wDeltaQ);

    CUDA_CHECK(cudaGetLastError());
}

void launchApplyDeltaPositions(DeviceParticles3D& particles)
{
    if (particles.count <= 0) return;

    applyDeltaPositionsKernel<<<CudaUtils::gridSize(particles.count), CudaUtils::BLOCK_SIZE>>>(
        particles.count,
        particles.x, particles.y, particles.z,
        particles.dx, particles.dy, particles.dz);

    CUDA_CHECK(cudaGetLastError());
}
