#include "simCPU/pbf/cpuPbfDensity.h"

#include <cmath>

#include "simCPU/utils/cpuSphKernels.h"

namespace CpuPBF
{
    void computeDensity(
        Particles3D& particles,
        const std::vector<int>& neighborOffsets,
        const std::vector<int>& neighborIds,
        float smoothingRadius)
    {
        const int n = particles.count;
        if (n <= 0)
            return;

        #pragma omp parallel for
        for (int i = 0; i < n; ++i)
        {
            const float xi = particles.x[i];
            const float yi = particles.y[i];
            const float zi = particles.z[i];

            float rho = particles.mass[i] * CpuSPH::poly6(0.0f, smoothingRadius);

            const int begin = neighborOffsets[i];
            const int end = neighborOffsets[i + 1];

            for (int k = begin; k < end; ++k)
            {
                const int j = neighborIds[k];

                const float dx = xi - particles.x[j];
                const float dy = yi - particles.y[j];
                const float dz = zi - particles.z[j];
                const float r = std::sqrt(dx * dx + dy * dy + dz * dz);

                rho += particles.mass[j] * CpuSPH::poly6(r, smoothingRadius);
            }

            particles.density[i] = rho;
        }
    }
}