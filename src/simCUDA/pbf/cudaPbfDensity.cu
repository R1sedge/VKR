#include "simCUDA/pbf/cudaPbfDensity.cuh"

#include <cuda_runtime.h>
#include <math.h>

#include "simCUDA/utils/cudaCheck.h"
#include "simCUDA/utils/cudaUtils.cuh"
#include "simCUDA/utils/cudaSphKernels.cuh"

namespace
{
    __global__ void computeDensityKernel(
        int n,
        const float* x,
        const float* y,
        const float* z,
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
        const float zi = z[i];

        float rho = mass[i] * CudaSPH::poly6(0.0f, h);

        const int begin = neighborOffsets[i];
        const int end   = neighborOffsets[i + 1];

        for (int k = begin; k < end; ++k)
        {
            const int j = neighborIds[k];

            const float dx = xi - x[j];
            const float dy = yi - y[j];
            const float dz = zi - z[j];
            const float r = sqrtf(dx * dx + dy * dy + dz * dz);

            rho += mass[j] * CudaSPH::poly6(r, h);
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

    computeDensityKernel<<<CudaUtils::gridSize(particles.count), CudaUtils::BLOCK_SIZE>>>(
        particles.count,
        particles.x,
        particles.y,
        particles.z,
        particles.mass,
        neighbors.offsets,
        neighbors.ids,
        particles.density,
        smoothingRadius);

    CUDA_CHECK(cudaGetLastError());
}
