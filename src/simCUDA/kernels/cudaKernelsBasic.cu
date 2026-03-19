#include "simCUDA/kernels/cudaKernelsBasic.cuh"

#include <cuda_runtime.h>

#include "simCUDA/utils/cudaCheck.h"
#include "simCUDA/utils/cudaUtils.cuh"

namespace
{
    __global__ void clearDerivedKernel(
        int n,
        float* density,
        float* lambda,
        float* dx,
        float* dy)
    {
        const int i = blockIdx.x * blockDim.x + threadIdx.x;
        if (i >= n) return;

        density[i] = 0.0f;
        lambda[i] = 0.0f;
        dx[i] = 0.0f;
        dy[i] = 0.0f;
    }

    __global__ void predictPositionKernel(
        int n,
        float* x, float* y,
        float* px, float* py,
        float* vx, float* vy,
        float dt,
        float gx, float gy,
        float velocityDamping)
    {
        const int i = blockIdx.x * blockDim.x + threadIdx.x;
        if (i >= n) return;

        const float damp = 1.0f - velocityDamping;

        vx[i] += gx * dt;
        vy[i] += gy * dt;

        vx[i] *= damp;
        vy[i] *= damp;

        px[i] = x[i];
        py[i] = y[i];

        x[i] += vx[i] * dt;
        y[i] += vy[i] * dt;
    }
    
    __global__ void updateVelocitiesKernel(
        int n,
        const float* x,
        const float* y,
        const float* px,
        const float* py,
        float* vx,
        float* vy,
        float invDt)
    {
        const int i = blockIdx.x * blockDim.x + threadIdx.x;
        if (i >= n) return;

        vx[i] = (x[i] - px[i]) * invDt;
        vy[i] = (y[i] - py[i]) * invDt;
    }
}

void launchClearDerived(DeviceParticles2D& dp)
{
    if (dp.count <= 0)
    return;

    clearDerivedKernel<<<CudaUtils::gridSize(dp.count), CudaUtils::BLOCK_SIZE>>>(
        dp.count,
        dp.density,
        dp.lambda,
        dp.dx,
        dp.dy);
        
    CUDA_CHECK(cudaGetLastError());
}

void launchPredictPositions(DeviceParticles2D& dp, float dt, float gx, float gy, float velocityDamping)
{
    if (dp.count <= 0)
    return;

    predictPositionKernel<<<CudaUtils::gridSize(dp.count), CudaUtils::BLOCK_SIZE>>>(
        dp.count,
        dp.x, dp.y,
        dp.px, dp.py,
        dp.vx, dp.vy,
        dt,
        gx, gy,
        velocityDamping);

    CUDA_CHECK(cudaGetLastError());
}

void launchUpdateVelocities(DeviceParticles2D& dp, float dt)
{
    if (dp.count <= 0 || dt <= 0.0f)
        return;

    const float invDt = 1.0f / dt;

    updateVelocitiesKernel<<<CudaUtils::gridSize(dp.count), CudaUtils::BLOCK_SIZE>>>(
        dp.count,
        dp.x, dp.y,
        dp.px, dp.py,
        dp.vx, dp.vy,
        invDt);

    CUDA_CHECK(cudaGetLastError());
}
