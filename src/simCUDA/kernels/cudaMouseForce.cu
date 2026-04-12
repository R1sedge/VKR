#include "cudaMouseForce.cuh"
#include <cuda_runtime.h>
#include "simCUDA/utils/cudaCheck.h"
#include "simCUDA/utils/cudaUtils.cuh"

namespace
{
    __global__ void applyMouseForceKernel(
        int n,
        const float* __restrict__ x,
        const float* __restrict__ y,
        float* __restrict__ vx,
        float* __restrict__ vy,
        float mouseX, float mouseY,
        float radius, float strength,
        int forceType)
    {
        const int i = blockIdx.x * blockDim.x + threadIdx.x;
        if (i >= n) return;

        const float dx = x[i] - mouseX;
        const float dy = y[i] - mouseY;
        const float distSq = dx * dx + dy * dy;
        const float radiusSq = radius * radius;

        if (distSq >= radiusSq || distSq < 1e-6f) return;

        const float dist = sqrtf(distSq);
        const float invDist = 1.0f / dist;
        const float falloff = 1.0f - (dist / radius);
        const float forceMag = strength * falloff;

        if (forceType == 0) {
            // Repulsion: push away
            vx[i] += forceMag * dx * invDist;
            vy[i] += forceMag * dy * invDist;
        }
        else if (forceType == 1) {
            // Attraction: pull toward
            vx[i] -= forceMag * dx * invDist;
            vy[i] -= forceMag * dy * invDist;
        }
        else if (forceType == 2) {
            // Vortex: rotate around (perpendicular)
            vx[i] += forceMag * (-dy) * invDist;
            vy[i] += forceMag * dx * invDist;
        }
    }
}

void launchApplyMouseForce(DeviceParticles2D& particles,
                          float mouseX, float mouseY,
                          float radius, float strength,
                          int forceType)
{
    if (particles.count <= 0) return;

    applyMouseForceKernel<<<CudaUtils::gridSize(particles.count), CudaUtils::BLOCK_SIZE>>>(
        particles.count,
        particles.x, particles.y,
        particles.vx, particles.vy,
        mouseX, mouseY,
        radius, strength,
        forceType);

    CUDA_CHECK(cudaGetLastError());
}
