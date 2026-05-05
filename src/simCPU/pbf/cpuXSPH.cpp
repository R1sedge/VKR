#include "simCPU/pbf/cpuXSPH.h"

#include <algorithm>
#include <cmath>

#include "simCPU/utils/cpuSphKernels.h"

namespace CpuXSPH
{
    void applyXSPH(
        Particles3D& particles,
        const std::vector<int>& neighborOffsets,
        const std::vector<int>& neighborIds,
        float xsphViscosity,
        float smoothingRadius)
    {
        const int n = particles.count;
        if (n <= 0 || xsphViscosity <= 0.0f)
            return;

        // Используем dx/dy/dz как временный буфер dv, как в CUDA.
        #pragma omp parallel for
        for (int i = 0; i < n; ++i)
        {
            const float xi = particles.x[i];
            const float yi = particles.y[i];
            const float zi = particles.z[i];

            const float vxi = particles.vx[i];
            const float vyi = particles.vy[i];
            const float vzi = particles.vz[i];

            float dvx = 0.0f;
            float dvy = 0.0f;
            float dvz = 0.0f;

            const int begin = neighborOffsets[i];
            const int end = neighborOffsets[i + 1];

            for (int k = begin; k < end; ++k)
            {
                const int j = neighborIds[k];

                const float dx = xi - particles.x[j];
                const float dy = yi - particles.y[j];
                const float dz = zi - particles.z[j];

                const float r = std::sqrt(dx * dx + dy * dy + dz * dz);
                const float w = CpuSPH::poly6(r, smoothingRadius);

                const float rhoJ = std::max(particles.density[j], 1e-6f);
                const float coeff = particles.mass[j] / rhoJ * w;

                dvx += coeff * (particles.vx[j] - vxi);
                dvy += coeff * (particles.vy[j] - vyi);
                dvz += coeff * (particles.vz[j] - vzi);
            }

            particles.dx[i] = dvx;
            particles.dy[i] = dvy;
            particles.dz[i] = dvz;
        }

        #pragma omp parallel for
        for (int i = 0; i < n; ++i)
        {
            particles.vx[i] += xsphViscosity * particles.dx[i];
            particles.vy[i] += xsphViscosity * particles.dy[i];
            particles.vz[i] += xsphViscosity * particles.dz[i];

            particles.dx[i] = 0.0f;
            particles.dy[i] = 0.0f;
            particles.dz[i] = 0.0f;
        }
    }
}