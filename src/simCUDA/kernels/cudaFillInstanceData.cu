#include "simCUDA/kernels/cudaFillInstanceData.cuh"

#include <cuda_runtime.h>
#include <math.h>

#include "simCUDA/utils/cudaCheck.h"
#include "simCUDA/utils/cudaUtils.cuh"

__global__ void fillInstanceDataKernel(
    int n,
    const float* __restrict__ x,
    const float* __restrict__ y,
    const float* __restrict__ z,
    const float* __restrict__ vx,
    const float* __restrict__ vy,
    float* __restrict__ out,    // layout: [x, y, z, radius, speed] * n
    float radius)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= n) return;

    float speed = sqrtf(vx[i] * vx[i] + vy[i] * vy[i]);
    out[i*5 + 0] = x[i];
    out[i*5 + 1] = y[i];
    out[i*5 + 2] = z ? z[i] : 0.0f;
    out[i*5 + 3] = radius;
    out[i*5 + 4] = speed;
}

void launchFillInstanceData(
    const DeviceParticles2D& dp,
    float* d_instanceBuffer,
    float radius)
{
    if (dp.count <= 0) return;

    fillInstanceDataKernel<<<CudaUtils::gridSize(dp.count), CudaUtils::BLOCK_SIZE>>>(
        dp.count, dp.x, dp.y, dp.z, dp.vx, dp.vy,
        d_instanceBuffer, radius);

    CUDA_CHECK(cudaGetLastError());
}
