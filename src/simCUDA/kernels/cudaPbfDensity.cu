#include "simCUDA/kernels/cudaPbfDensity.cuh"

#include <cuda_runtime.h>
#include <math.h>

#include "simCUDA/cudaCheck.h"

namespace
{
    constexpr int BLOCK_SIZE = 256;

    int gridSize(int n)
    {
        return (n + BLOCK_SIZE - 1) / BLOCK_SIZE;
    }

    __device__ float poly6Kernel(float r, float h)
    {
        if (r < 0.0f || r > h)
            return 0.0f;

        const float kPi = 3.14159265358979323846f;
        const float h2 = h * h;
        const float h4 = h2 * h2;
        const float h8 = h4 * h4;
        const float k = 4.0f / (kPi * h8);

        const float x = h2 - r * r;
        return k * x * x * x;
    }

    __global__ void computeDensityKernel(
        int n,
        const float* x,
        const float* y,
        const float* mass,
        const int* neighborOffsets,
        const int* neighborIds,
        float* density,
        float h)
    {
        const int i = blockIdx.x * blockDim.x + threadIdx.x;
        if (i >= n) return;

        const float xi = x[i];
        const float yi = y[i];

        float rho = mass[i] * poly6Kernel(0.0f, h);

        const int begin = neighborOffsets[i];
        const int end   = neighborOffsets[i + 1];

        for (int k = begin; k < end; ++k)
        {
            const int j = neighborIds[k];

            const float dx = xi - x[j];
            const float dy = yi - y[j];
            const float r = sqrtf(dx * dx + dy * dy);

            rho += mass[j] * poly6Kernel(r, h);
        }

        density[i] = rho;
    }
}

void launchComputeDensity(
    const DeviceParticles2D& particles,
    const DeviceNeighborList& neighbors,
    float smoothingRadius)
{
    if (particles.count <= 0)
        return;
    
    computeDensityKernel<<<gridSize(particles.count), BLOCK_SIZE>>>(
        particles.count,
        particles.x,
        particles.y,
        particles.mass,
        neighbors.offsets,
        neighbors.ids,
        particles.density,
        smoothingRadius);

    CUDA_CHECK(cudaGetLastError());
}
