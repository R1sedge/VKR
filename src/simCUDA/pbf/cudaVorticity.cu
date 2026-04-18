#include "cudaVorticity.cuh"
#include <cuda_runtime.h>
#include <math.h>
#include "simCUDA/utils/cudaCheck.h"
#include "simCUDA/utils/cudaUtils.cuh"
#include "simCUDA/utils/cudaSphKernels.cuh"

namespace
{
    // 3D curl: ω_i = Σ_j (m_j/rho_j) * (v_j - v_i) × gradW
    __global__
    void computeVorticityKernel(
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
        float*       __restrict__ omegaX,
        float*       __restrict__ omegaY,
        float*       __restrict__ omegaZ,
        float h)
    {
        const int i = blockIdx.x * blockDim.x + threadIdx.x;
        if (i >= n) return;

        const float xi = x[i], yi = y[i], zi = z[i];
        const float vxi = vx[i], vyi = vy[i], vzi = vz[i];

        float omX = 0.0f, omY = 0.0f, omZ = 0.0f;

        const int begin = neighborOffsets[i];
        const int end = neighborOffsets[i + 1];

        for (int k = begin; k < end; ++k)
        {
            const int j = neighborIds[k];

            const float dx = xi - x[j];
            const float dy = yi - y[j];
            const float dz = zi - z[j];

            const float r2 = fmaf(dx, dx, fmaf(dy, dy, dz * dz));
            if (r2 < 1e-12f) continue;

            const float invR = rsqrtf(r2);
            const float r = r2 * invR;

            const float gradW  = CudaSPH::spikyGradCoeff(r, h);
            const float gradWx = gradW * dx * invR;
            const float gradWy = gradW * dy * invR;
            const float gradWz = gradW * dz * invR;

            const float invRhoJ = __frcp_rn(fmaxf(density[j], 1e-6f));
            const float massOverRho = mass[j] * invRhoJ;

            const float dvx = vx[j] - vxi;
            const float dvy = vy[j] - vyi;
            const float dvz = vz[j] - vzi;

            // cross = dv × gradW
            const float crossX = dvy * gradWz - dvz * gradWy;
            const float crossY = dvz * gradWx - dvx * gradWz;
            const float crossZ = dvx * gradWy - dvy * gradWx;

            omX = fmaf(massOverRho, crossX, omX);
            omY = fmaf(massOverRho, crossY, omY);
            omZ = fmaf(massOverRho, crossZ, omZ);
        }

        omegaX[i] = omX;
        omegaY[i] = omY;
        omegaZ[i] = omZ;
    }

    // 3D confinement: f_vort = ε * (N × ω_i), N = normalize(Σ (m_j/rho_j) * |ω_j| * gradW)
    __global__
    void applyVorticityConfinementKernel(
        int n,
        const float* __restrict__ x,
        const float* __restrict__ y,
        const float* __restrict__ z,
        float*       __restrict__ vx,
        float*       __restrict__ vy,
        float*       __restrict__ vz,
        const float* __restrict__ mass,
        const float* __restrict__ density,
        const float* __restrict__ omegaX,
        const float* __restrict__ omegaY,
        const float* __restrict__ omegaZ,
        const int*   __restrict__ neighborOffsets,
        const int*   __restrict__ neighborIds,
        float dt,
        float epsilon,
        float h)
    {
        const int i = blockIdx.x * blockDim.x + threadIdx.x;
        if (i >= n) return;

        const float xi = x[i], yi = y[i], zi = z[i];
        float etaX = 0.0f, etaY = 0.0f, etaZ = 0.0f;

        const int begin = neighborOffsets[i];
        const int end   = neighborOffsets[i + 1];

        for (int k = begin; k < end; ++k)
        {
            const int j = neighborIds[k];

            const float dx = xi - x[j];
            const float dy = yi - y[j];
            const float dz = zi - z[j];
            const float r2 = fmaf(dx, dx, fmaf(dy, dy, dz * dz));

            const float invR = rsqrtf(r2);
            const float r = r2 * invR;

            const float gradW  = CudaSPH::spikyGradCoeff(r, h);
            const float gradWx = gradW * dx * invR;
            const float gradWy = gradW * dy * invR;
            const float gradWz = gradW * dz * invR;

            const float invRhoJ = __frcp_rn(fmaxf(density[j], 1e-6f));
            // |ω_j|
            const float omegaMag = sqrtf(fmaf(omegaX[j], omegaX[j], fmaf(omegaY[j], omegaY[j], omegaZ[j] * omegaZ[j])));
            // coeff = (m_j / rho_j) * |ω_j|
            const float coeff = mass[j] * invRhoJ * omegaMag;

            etaX = fmaf(coeff, gradWx, etaX);
            etaY = fmaf(coeff, gradWy, etaY);
            etaZ = fmaf(coeff, gradWz, etaZ);
        }

        // Нормируем: N = η / |η|
        const float eta2 = fmaf(etaX, etaX, fmaf(etaY, etaY, etaZ * etaZ));
        if (eta2 < 1e-12f) return;

        const float invLen = rsqrtf(eta2);
        const float Nx = etaX * invLen;
        const float Ny = etaY * invLen;
        const float Nz = etaZ * invLen;

        // f_vort = ε * (N × ω_i)
        const float omXi = omegaX[i];
        const float omYi = omegaY[i];
        const float omZi = omegaZ[i];

        const float fvx = Ny * omZi - Nz * omYi;
        const float fvy = Nz * omXi - Nx * omZi;
        const float fvz = Nx * omYi - Ny * omXi;

        vx[i] = fmaf(epsilon * dt, fvx, vx[i]);
        vy[i] = fmaf(epsilon * dt, fvy, vy[i]);
        vz[i] = fmaf(epsilon * dt, fvz, vz[i]);
    }
}

void launchComputeVorticity(
    const DeviceParticles3D& particles,
    const DeviceNeighborList& neighbors,
    float smoothingRadius)
{
    if (particles.count == 0) return;

    computeVorticityKernel<<<CudaUtils::gridSize(particles.count), CudaUtils::BLOCK_SIZE>>>(
        particles.count,
        particles.x, particles.y, particles.z,
        particles.vx, particles.vy, particles.vz,
        particles.mass, particles.density,
        neighbors.offsets, neighbors.ids,
        particles.omegaX, particles.omegaY, particles.omegaZ,
        smoothingRadius);

    CUDA_CHECK(cudaGetLastError());
}

void launchApplyVorticityConfinement(
    DeviceParticles3D& particles,
    const DeviceNeighborList& neighbors,
    float dt,
    float vorticityEpsilon,
    float smoothingRadius)
{
    if (particles.count == 0 || vorticityEpsilon == 0.0f) return;

    applyVorticityConfinementKernel<<<CudaUtils::gridSize(particles.count), CudaUtils::BLOCK_SIZE>>>(
        particles.count,
        particles.x, particles.y, particles.z,
        particles.vx, particles.vy, particles.vz,
        particles.mass, particles.density,
        particles.omegaX, particles.omegaY, particles.omegaZ,
        neighbors.offsets, neighbors.ids,
        dt,
        vorticityEpsilon,
        smoothingRadius);

    CUDA_CHECK(cudaGetLastError());
}
