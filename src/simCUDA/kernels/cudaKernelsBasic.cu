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
        float* dy,
        float* dz)
    {
        const int i = blockIdx.x * blockDim.x + threadIdx.x;
        if (i >= n) return;

        density[i] = 0.0f;
        lambda[i] = 0.0f;
        dx[i] = 0.0f;
        dy[i] = 0.0f;
        dz[i] = 0.0f;
    }

    __global__ void predictPositionKernel(
        int n,
        float* x, float* y, float* z,
        float* px, float* py, float* pz,
        float* vx, float* vy, float* vz,
        float dt,
        float gx, float gy, float gz,
        float velocityDamping)
    {
        const int i = blockIdx.x * blockDim.x + threadIdx.x;
        if (i >= n) return;

        const float damp = 1.0f - velocityDamping;

        vx[i] += gx * dt;
        vy[i] += gy * dt;
        vz[i] += gz * dt;

        vx[i] *= damp;
        vy[i] *= damp;
        vz[i] *= damp;

        px[i] = x[i];
        py[i] = y[i];
        pz[i] = z[i];

        x[i] += vx[i] * dt;
        y[i] += vy[i] * dt;
        z[i] += vz[i] * dt;
    }
    
    __global__ void updateVelocitiesKernel(
        int n,
        const float* x,
        const float* y,
        const float* z,
        const float* px,
        const float* py,
        const float* pz,
        float* vx,
        float* vy,
        float* vz,
        float invDt,
        float maxSpeed,
        float radius)
    {
        const int i = blockIdx.x * blockDim.x + threadIdx.x;
        if (i >= n) return;

        float newVx = (x[i] - px[i]) * invDt;
        float newVy = (y[i] - py[i]) * invDt;
        float newVz = (z[i] - pz[i]) * invDt;

        float speedSq = newVx * newVx + newVy * newVy + newVz * newVz;
        if (speedSq > maxSpeed * maxSpeed)
        {
            float invSpeed = maxSpeed * rsqrtf(speedSq);
            newVx *= invSpeed;
            newVy *= invSpeed;
            newVz *= invSpeed;
        }

        vx[i] = newVx;
        vy[i] = newVy;
        vz[i] = newVz;
    }
}

void launchClearDerived(DeviceParticles3D& dp)
{
    if (dp.count <= 0)
    return;

    clearDerivedKernel<<<CudaUtils::gridSize(dp.count), CudaUtils::BLOCK_SIZE>>>(
        dp.count,
        dp.density,
        dp.lambda,
        dp.dx,
        dp.dy,
        dp.dz);

    CUDA_CHECK(cudaGetLastError());
}

void launchPredictPositions(DeviceParticles3D& dp, float dt, float gx, float gy, float gz, float velocityDamping)
{
    if (dp.count <= 0)
    return;

    predictPositionKernel<<<CudaUtils::gridSize(dp.count), CudaUtils::BLOCK_SIZE>>>(
        dp.count,
        dp.x, dp.y, dp.z,
        dp.px, dp.py, dp.pz,
        dp.vx, dp.vy, dp.vz,
        dt,
        gx, gy, gz,
        velocityDamping);

    CUDA_CHECK(cudaGetLastError());
}

void launchUpdateVelocities(DeviceParticles3D& dp, float dt, float maxSpeed, float radius)
{
    if (dp.count <= 0 || dt <= 0.0f)
        return;

    const float invDt = 1.0f / dt;

    updateVelocitiesKernel<<<CudaUtils::gridSize(dp.count), CudaUtils::BLOCK_SIZE>>>(
        dp.count,
        dp.x, dp.y, dp.z,
        dp.px, dp.py, dp.pz,
        dp.vx, dp.vy, dp.vz,
        invDt, maxSpeed, radius);

    CUDA_CHECK(cudaGetLastError());
}
