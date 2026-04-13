#include "cudaXSPH.cuh"
#include <cuda_runtime.h>
#include <math.h>
#include "simCUDA/utils/cudaCheck.h"
#include "simCUDA/utils/cudaUtils.cuh"
#include "simCUDA/utils/cudaSphKernels.cuh"

namespace {

    // Накапливаем поправку в dx/dy/dz
    __global__
    void accumulateXSPHKernel(
        int n,
        const float* __restrict__ x,
        const float* __restrict__ y,
        const float* __restrict__ z,
        const float* __restrict__ vx,
        const float* __restrict__ vy,
        const float* __restrict__ vz,
        const float* __restrict__ mass,
        const float* __restrict__ density,
        const int*   __restrict__ neighborOffsets,
        const int*   __restrict__ neighborIds,
        float* __restrict__ dvxOut,   // == particles.dx, обнулены заранее
        float* __restrict__ dvyOut,   // == particles.dy
        float* __restrict__ dvzOut,   // == particles.dz
        float h)
    {
        const int i = blockIdx.x * blockDim.x + threadIdx.x;
        if (i >= n) return;

        const float xi = x[i], yi = y[i], zi = z[i];
        const float vxi = vx[i], vyi = vy[i], vzi = vz[i];

        float dvx = 0.0f, dvy = 0.0f, dvz = 0.0f;

        const int begin = neighborOffsets[i];
        const int end = neighborOffsets[i + 1];

        for (int k = begin; k < end; ++k)
        {
            const int j = neighborIds[k];

            const float dx = xi - x[j];
            const float dy = yi - y[j];
            const float dz = zi - z[j];
            const float r = sqrtf(dx * dx + dy * dy + dz * dz);

            const float W = CudaSPH::poly6(r, h);
            const float rhoJ = fmaxf(density[j], 1e-6f);
            const float coeff = mass[j] / rhoJ * W;

            dvx += coeff * (vx[j] - vxi);
            dvy += coeff * (vy[j] - vyi);
            dvz += coeff * (vz[j] - vzi);
        }

        dvxOut[i] = dvx;
        dvyOut[i] = dvy;
        dvzOut[i] = dvz;
    }

    // Применяем поправку
    __global__
    void applyXSPHKernel(
        int n,
        float* __restrict__ vx,
        float* __restrict__ vy,
        float* __restrict__ vz,
        float* __restrict__ dx,    // сбрасываем в 0 после применения
        float* __restrict__ dy,
        float* __restrict__ dz,
        float c)
    {
        const int i = blockIdx.x * blockDim.x + threadIdx.x;
        if (i >= n) return;

        vx[i] += c * dx[i];
        vy[i] += c * dy[i];
        vz[i] += c * dz[i];
        dx[i] = 0.0f;  // Возвращаем буфер в чистое состояние
        dy[i] = 0.0f;
        dz[i] = 0.0f;
    }
}

void launchApplyXSPH(DeviceParticles2D& particles,
                     const DeviceNeighborList& neighbors,
                     float xsphViscosity,
                     float smoothingRadius)
{
    if (particles.count == 0 || xsphViscosity <= 0.0f) return;

    accumulateXSPHKernel<<<CudaUtils::gridSize(particles.count), CudaUtils::BLOCK_SIZE>>>(
        particles.count,
        particles.x, particles.y, particles.z,
        particles.vx, particles.vy, particles.vz,
        particles.mass, particles.density,
        neighbors.offsets, neighbors.ids,
        particles.dx, particles.dy, particles.dz,
        smoothingRadius);

    CUDA_CHECK(cudaGetLastError());

    applyXSPHKernel<<<CudaUtils::gridSize(particles.count), CudaUtils::BLOCK_SIZE>>>(
         particles.count,
        particles.vx, particles.vy, particles.vz,
        particles.dx, particles.dy, particles.dz,
        xsphViscosity);

    CUDA_CHECK(cudaGetLastError());
}