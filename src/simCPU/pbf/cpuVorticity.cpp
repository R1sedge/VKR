#include "simCPU/pbf/cpuVorticity.h"

#include <algorithm>
#include <cmath>

#include "simCPU/utils/cpuSphKernels.h"

namespace CpuVorticity
{
    void computeVorticity(
        Particles3D& particles,
        const std::vector<int>& neighborOffsets,
        const std::vector<int>& neighborIds,
        float smoothingRadius)
    {
        const int n = particles.count;
        if (n <= 0)
            return;

        if (static_cast<int>(particles.omegaX.size()) != n)
        {
            particles.omegaX.assign(n, 0.0f);
            particles.omegaY.assign(n, 0.0f);
            particles.omegaZ.assign(n, 0.0f);
        }

        #pragma omp parallel for
        for (int i = 0; i < n; ++i)
        {
            const float xi = particles.x[i];
            const float yi = particles.y[i];
            const float zi = particles.z[i];

            const float vxi = particles.vx[i];
            const float vyi = particles.vy[i];
            const float vzi = particles.vz[i];

            float omX = 0.0f;
            float omY = 0.0f;
            float omZ = 0.0f;

            const int begin = neighborOffsets[i];
            const int end = neighborOffsets[i + 1];

            for (int k = begin; k < end; ++k)
            {
                const int j = neighborIds[k];

                const float dx = xi - particles.x[j];
                const float dy = yi - particles.y[j];
                const float dz = zi - particles.z[j];

                const float r2 = dx * dx + dy * dy + dz * dz;
                if (r2 < 1e-12f)
                    continue;

                const float r = std::sqrt(r2);
                const float invR = 1.0f / r;

                const float gradW = CpuSPH::spikyGradCoeff(r, smoothingRadius);
                const float gradWx = gradW * dx * invR;
                const float gradWy = gradW * dy * invR;
                const float gradWz = gradW * dz * invR;

                const float invRhoJ = 1.0f / std::max(particles.density[j], 1e-6f);
                const float massOverRho = particles.mass[j] * invRhoJ;

                const float dvx = particles.vx[j] - vxi;
                const float dvy = particles.vy[j] - vyi;
                const float dvz = particles.vz[j] - vzi;

                // omega_i += (m_j / rho_j) * ((v_j - v_i) x gradW)
                const float crossX = dvy * gradWz - dvz * gradWy;
                const float crossY = dvz * gradWx - dvx * gradWz;
                const float crossZ = dvx * gradWy - dvy * gradWx;

                omX += massOverRho * crossX;
                omY += massOverRho * crossY;
                omZ += massOverRho * crossZ;
            }

            particles.omegaX[i] = omX;
            particles.omegaY[i] = omY;
            particles.omegaZ[i] = omZ;
        }
    }

    void applyVorticityConfinement(
        Particles3D& particles,
        const std::vector<int>& neighborOffsets,
        const std::vector<int>& neighborIds,
        float dt,
        float vorticityEpsilon,
        float smoothingRadius)
    {
        const int n = particles.count;
        if (n <= 0 || dt <= 0.0f || vorticityEpsilon == 0.0f)
            return;

        #pragma omp parallel for
        for (int i = 0; i < n; ++i)
        {
            const float xi = particles.x[i];
            const float yi = particles.y[i];
            const float zi = particles.z[i];

            float etaX = 0.0f;
            float etaY = 0.0f;
            float etaZ = 0.0f;

            const int begin = neighborOffsets[i];
            const int end = neighborOffsets[i + 1];

            for (int k = begin; k < end; ++k)
            {
                const int j = neighborIds[k];

                const float dx = xi - particles.x[j];
                const float dy = yi - particles.y[j];
                const float dz = zi - particles.z[j];

                const float r2 = dx * dx + dy * dy + dz * dz;
                if (r2 < 1e-12f)
                    continue;

                const float r = std::sqrt(r2);
                const float invR = 1.0f / r;

                const float gradW = CpuSPH::spikyGradCoeff(r, smoothingRadius);
                const float gradWx = gradW * dx * invR;
                const float gradWy = gradW * dy * invR;
                const float gradWz = gradW * dz * invR;

                const float invRhoJ = 1.0f / std::max(particles.density[j], 1e-6f);

                const float omegaMag = std::sqrt(
                    particles.omegaX[j] * particles.omegaX[j] +
                    particles.omegaY[j] * particles.omegaY[j] +
                    particles.omegaZ[j] * particles.omegaZ[j]);

                const float coeff = particles.mass[j] * invRhoJ * omegaMag;

                etaX += coeff * gradWx;
                etaY += coeff * gradWy;
                etaZ += coeff * gradWz;
            }

            const float eta2 = etaX * etaX + etaY * etaY + etaZ * etaZ;
            if (eta2 < 1e-12f)
                continue;

            const float invLen = 1.0f / std::sqrt(eta2);
            const float nx = etaX * invLen;
            const float ny = etaY * invLen;
            const float nz = etaZ * invLen;

            const float omX = particles.omegaX[i];
            const float omY = particles.omegaY[i];
            const float omZ = particles.omegaZ[i];

            // f_vort = epsilon * (N x omega_i)
            const float fvx = ny * omZ - nz * omY;
            const float fvy = nz * omX - nx * omZ;
            const float fvz = nx * omY - ny * omX;

            const float scale = vorticityEpsilon * dt;

            particles.vx[i] += scale * fvx;
            particles.vy[i] += scale * fvy;
            particles.vz[i] += scale * fvz;
        }
    }
}