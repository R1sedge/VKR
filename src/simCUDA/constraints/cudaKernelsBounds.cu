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
        float* z,
        float left,
        float right,
        float bottom,
        float top,
        float front,
        float back,
        float radius)
    {
        const int i = blockIdx.x * blockDim.x + threadIdx.x;
        if (i >= n) return;

        if (x[i] < left + radius)   x[i] = left + radius;
        if (x[i] > right - radius)  x[i] = right - radius;
        if (y[i] < bottom + radius) y[i] = bottom + radius;
        if (y[i] > top - radius)    y[i] = top - radius;
        if (z[i] < front + radius)  z[i] = front + radius;
        if (z[i] > back - radius)   z[i] = back - radius;
    }

    __global__ void projectToVesselPlanesKernel(
        int n,
        float* __restrict__ x,
        float* __restrict__ y,
        float* __restrict__ z,
        const DeviceBoundaryPlane* __restrict__ planes,
        int planeCount,
        float radius)
    {
        const int i = blockIdx.x * blockDim.x + threadIdx.x;
        if (i >= n) return;

        float px = x[i];
        float py = y[i];
        float pz = z[i];

        // Для inward-facing planes считаем particle center валидным,
        // если signedDist >= radius. Иначе двигаем центр по нормали внутрь.
        for (int planeIdx = 0; planeIdx < planeCount; ++planeIdx)
        {
            const DeviceBoundaryPlane plane = planes[planeIdx];

            const float dist =
                plane.nx * (px - plane.px) +
                plane.ny * (py - plane.py) +
                plane.nz * (pz - plane.pz);

            if (dist < radius)
            {
                const float correction = radius - dist;
                px += correction * plane.nx;
                py += correction * plane.ny;
                pz += correction * plane.nz;
            }
        }

        x[i] = px;
        y[i] = py;
        z[i] = pz;
    }
}

void launchProjectBounds(DeviceParticles3D& dp,
                         float left,
                         float right,
                         float bottom,
                         float top,
                         float front,
                         float back,
                         float radius)
{
    if (dp.count <= 0)
        return;

    projectBoundsKernel<<<CudaUtils::gridSize(dp.count), CudaUtils::BLOCK_SIZE>>>(
        dp.count,
        dp.x,
        dp.y,
        dp.z,
        left,
        right,
        bottom,
        top,
        front,
        back,
        radius);

    CUDA_CHECK(cudaGetLastError());
}

void launchProjectToVesselPlanes(DeviceParticles3D& dp,
                                 const DeviceBoundaryPlane* planes,
                                 int planeCount,
                                 float radius)
{
    if (dp.count <= 0 || planes == nullptr || planeCount <= 0)
        return;

    projectToVesselPlanesKernel<<<CudaUtils::gridSize(dp.count), CudaUtils::BLOCK_SIZE>>>(
        dp.count,
        dp.x,
        dp.y,
        dp.z,
        planes,
        planeCount,
        radius);

    CUDA_CHECK(cudaGetLastError());
}
