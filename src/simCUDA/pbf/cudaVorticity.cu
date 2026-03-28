#include "cudaVorticity.cuh"
#include <cuda_runtime.h>
#include <math.h>
#include "simCUDA/utils/cudaCheck.h"
#include "simCUDA/utils/cudaUtils.cuh"
#include "simCUDA/utils/cudaSphKernels.cuh"

namespace
{
    __global__ //__launch_bounds__(256, 2)
    void computeVorticityKernel(
    int n,
    const float* __restrict__ x,
    const float* __restrict__ y,
    const float* __restrict__ vx,
    const float* __restrict__ vy,
    const float* __restrict__ mass,
    const float* __restrict__ density,
    const int*   __restrict__ neighborOffsets,
    const int*   __restrict__ neighborIds,
    float*       __restrict__ omega,
    float h)
    {
        const int i = blockIdx.x * blockDim.x + threadIdx.x;
        if (i >= n) return;

        const float xi  = x[i], yi  = y[i];
        const float vxi = vx[i], vyi = vy[i];
        float curl = 0.0f;

        const int begin = neighborOffsets[i];
        const int end = neighborOffsets[i + 1];

        for (int k = begin; k < end; ++k) 
        {
            const int j = neighborIds[k];

            const float dx = xi - x[j];
            const float dy =  yi - y[j];

            const float r2 = fmaf(dx, dx, dy * dy);
            if (r2 < 1e-12f) continue;

            const float invR = rsqrtf(r2);
            const float r = r2 * invR;

            const float gradW  = CudaSPH::spikyGradCoeff(r, h);
            const float gradWx = gradW * dx * invR;
            const float gradWy = gradW * dy * invR;

            const float invRhoJ = __frcp_rn(fmaxf(density[j], 1e-6f));
            const float massOverRho = mass[j] * invRhoJ;

            const float dvx = vx[j] - vxi;
            const float dvy = vy[j] - vyi;

            // (dv × gradW) = dvx*gradWy - dvy*gradWx для 2D
            const float cross = fmaf(dvx, gradWy, -(dvy * gradWx));
            curl = fmaf(massOverRho, cross, curl);
        }

        omega[i] = curl;
    }

    __global__ //__launch_bounds__(256, 2)
    void applyVorticityConfinementKernel(
        int n,
        const float* __restrict__ x,
        const float* __restrict__ y,
        float*       __restrict__ vx,
        float*       __restrict__ vy,
        const float* __restrict__ mass,
        const float* __restrict__ density,
        const float* __restrict__ omega, // read-only в этом ядре
        const int*   __restrict__ neighborOffsets,
        const int*   __restrict__ neighborIds,
        float dt,
        float epsilon,
        float h)
    {
        const int i = blockIdx.x * blockDim.x + threadIdx.x;
        if (i >= n) return;

        const float xi = x[i], yi = y[i];
        float etaX = 0.0f, etaY = 0.0f;

        const int begin = neighborOffsets[i];
        const int end   = neighborOffsets[i + 1];

        for (int k = begin; k < end; ++k) 
        {
            const int j = neighborIds[k];

            const float dx = xi - x[j];
            const float dy = yi - y[j];
            const float r2 = fmaf(dx, dx, dy * dy);

            const float invR = rsqrtf(r2);
            const float r = r2 * invR;

            const float gradW  = CudaSPH::spikyGradCoeff(r, h);
            const float gradWx = gradW * dx * invR;
            const float gradWy = gradW * dy * invR;

            const float invRhoJ = __frcp_rn(fmaxf(density[j], 1e-6f));
            // coeff = (m_j / rho_j) * |omega_j|
            const float coeff = mass[j] * invRhoJ * fabsf(omega[j]);

            // FMA накопление
            etaX = fmaf(coeff, gradWx, etaX);
            etaY = fmaf(coeff, gradWy, etaY);
        }
        // Нормируем: N = η / |η|
        const float eta2 = fmaf(etaX, etaX, etaY * etaY);
        if (eta2 < 1e-12f) return;

        const float invLen = rsqrtf(eta2);
        const float Nx = etaX * invLen;
        const float Ny = etaY * invLen;

        // f = ε * ω_i * (Ny, -Nx)
        const float omegaI  = omega[i];
        const float dtEpsOm = epsilon * omegaI;

        vx[i] = fmaf(dtEpsOm,  Ny, vx[i]);
        vy[i] = fmaf(dtEpsOm, -Nx, vy[i]);
    }
}

void launchComputeVorticity(
    const DeviceParticles2D& particles,
    const DeviceNeighborList& neighbors,
    float smoothingRadius)
{
    if (particles.count == 0) return;

    computeVorticityKernel<<<CudaUtils::gridSize(particles.count), CudaUtils::BLOCK_SIZE>>>(
        particles.count,
        particles.x, particles.y,
        particles.vx, particles.vy,
        particles.mass, particles.density,
        neighbors.offsets, neighbors.ids,
        particles.omega,
        smoothingRadius);

    CUDA_CHECK(cudaGetLastError());
}

void launchApplyVorticityConfinement(
    DeviceParticles2D& particles,
    const DeviceNeighborList& neighbors,
    float dt,
    float vorticityEpsilon,
    float smoothingRadius)
{
    if (particles.count == 0 || vorticityEpsilon == 0.0f) return;

    applyVorticityConfinementKernel<<<CudaUtils::gridSize(particles.count), CudaUtils::BLOCK_SIZE>>>(
        particles.count,
        particles.x, particles.y,
        particles.vx, particles.vy,
        particles.mass, particles.density,
        particles.omega,
        neighbors.offsets, neighbors.ids,
        dt,
        vorticityEpsilon,
        smoothingRadius);

    CUDA_CHECK(cudaGetLastError());
}
