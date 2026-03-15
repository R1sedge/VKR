#include "simCUDA/constraints/cudaKernelsBounds.cuh"

#include <cuda_runtime.h>

#include "simCUDA/cudaCheck.h"
namespace
{
    constexpr int BLOCK_SIZE = 256;

    int gridSize(int n)
    {
        return (n + BLOCK_SIZE - 1) / BLOCK_SIZE;
    }

    __global__ void projectBoundsKernel(
        int n, 
        float* x,
        float* y,
        float left, 
        float right,
        float bottom,
        float top,
        float radius)
    {
        const int i = blockIdx.x * blockDim.x + threadIdx.x;
        if (i >= n) return;

        if (x[i] < left + radius)   x[i] = left + radius;
        if (x[i] > right - radius)  x[i] = right - radius;
        if (y[i] < bottom + radius) y[i] = bottom + radius;
        if (y[i] > top - radius)    y[i] = top - radius;
    }
}

void launchProjectBounds(
    DeviceParticles2D& dp,
    float left,
    float right,
    float bottom,
    float top,
    float radius)
{
    if (dp.count <= 0)
        return;

    projectBoundsKernel<<<gridSize(dp.count), BLOCK_SIZE>>>(
        dp.count,
        dp.x,
        dp.y,
        left,
        right,
        bottom,
        top,
        radius);

    CUDA_CHECK(cudaGetLastError());
}
