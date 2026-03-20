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
        const float* x,
        const float* y,
        const float* mass,
        const float* lambda,
        const int* neighborOffsets,
        const int* neighborIds,
        float* dxOut,
        float* dyOut,
        float restDensity,
        float h)
    {
        const int i = blockIdx.x * blockDim.x + threadIdx.x;
        if (i >= n) return;

        const float invRestDensity = 1.0f / restDensity;
        const float gradEps = 1e-6f;

        const float xi = x[i];
        const float yi = y[i];
        const float lambdaI = lambda[i];

        float deltaX = 0.0f;
        float deltaY = 0.0f;

        const int begin = neighborOffsets[i];
        const int end   = neighborOffsets[i + 1];

        for (int k = begin; k < end; ++k)
        {
            const int j = neighborIds[k];

            const float dx = xi - x[j];
            const float dy = yi - y[j];
            const float r2 = dx * dx + dy * dy;

            if (r2 < gradEps)
                continue;

            const float r = sqrtf(r2);
            const float invR = 1.0f / r;
            const float gradW = CudaSPH::spikyGradCoeff(r, h);

            const float coeff = (lambdaI + lambda[j]) * mass[j] * invRestDensity * gradW;

            deltaX += coeff * dx * invR;
            deltaY += coeff * dy * invR;
        }

        dxOut[i] = deltaX;
        dyOut[i] = deltaY;
    }

    __global__ void applyDeltaPositionsKernel(
        int n,
        float* x,
        float* y,
        float* dx,
        float* dy,
        float scale)
    {
        const int i = blockIdx.x * blockDim.x + threadIdx.x;
        if (i >= n) return;

        x[i] += scale * dx[i];
        y[i] += scale * dy[i];

        dx[i] = 0.0f;
        dy[i] = 0.0f;
    }
}

void launchComputeDeltaPositions(
    const DeviceParticles2D& particles,
    const DeviceNeighborList& neighbors,
    float restDensity,
    float smoothingRadius)
{
    if (particles.count <= 0)
        return;

    computeDeltaPositionsKernel<<<CudaUtils::gridSize(particles.count), CudaUtils::BLOCK_SIZE>>>(
        particles.count,
        particles.x,
        particles.y,
        particles.mass,
        particles.lambda,
        neighbors.offsets,
        neighbors.ids,
        particles.dx,
        particles.dy,
        restDensity,
        smoothingRadius);

    CUDA_CHECK(cudaGetLastError());
}

void launchApplyDeltaPositions(
    DeviceParticles2D& particles,
    float scale)
{
    if (particles.count <= 0)
        return;

    applyDeltaPositionsKernel<<<CudaUtils::gridSize(particles.count), CudaUtils::BLOCK_SIZE>>>(
        particles.count,
        particles.x,
        particles.y,
        particles.dx,
        particles.dy,
        scale);

    CUDA_CHECK(cudaGetLastError());
}
