#include "simCUDA/constraints/cudaKernelsBounds.cuh"

#include <cuda_runtime.h>

#include "simCUDA/utils/cudaCheck.h"
#include "simCUDA/utils/cudaUtils.cuh"
namespace
{
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

    projectBoundsKernel<<<CudaUtils::gridSize(dp.count), CudaUtils::BLOCK_SIZE>>>(
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
