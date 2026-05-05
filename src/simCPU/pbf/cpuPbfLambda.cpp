#include "simCPU/pbf/cpuPbfLambda.h"

#include <algorithm>
#include <cmath>

#include "simCPU/utils/cpuSphKernels.h"

namespace CpuPBF
{
    void computeLambda(
        Particles3D& particles,
        const std::vector<int>& neighborOffsets,
        const std::vector<int>& neighborIds,
        float restDensity,
        float epsilon,
        float smoothingRadius)
    {
        const int n = particles.count;
        if (n <= 0)
            return;

        const float invRestDensity = 1.0f / restDensity;
        const float h = smoothingRadius;

        #pragma omp parallel for
        for (int i = 0; i < n; ++i)
        {
            const float rhoI = particles.density[i];
            const float constraint = rhoI * invRestDensity - 1.0f;

            const float xi = particles.x[i];
            const float yi = particles.y[i];
            const float zi = particles.z[i];

            float sumGrad2 = 0.0f;
            float gradCiX = 0.0f;
            float gradCiY = 0.0f;
            float gradCiZ = 0.0f;

            const int begin = neighborOffsets[i];
            const int end = neighborOffsets[i + 1];

            for (int k = begin; k < end; ++k)
            {
                const int j = neighborIds[k];

                const float dx = xi - particles.x[j];
                const float dy = yi - particles.y[j];
                const float dz = zi - particles.z[j];

                float fdx = dx;
                float fdy = dy;
                float fdz = dz;

                float r2 = fdx * fdx + fdy * fdy + fdz * fdz;

                // CUDA-compatible soft-min path for coincident particles.
                if (r2 < 1e-12f)
                {
                    const unsigned int seed =
                        static_cast<unsigned int>(i) ^
                        (static_cast<unsigned int>(j) * 2654435761u);

                    fdx = static_cast<float>((seed >> 0u) & 0xFFu) - 127.5f;
                    fdy = static_cast<float>((seed >> 8u) & 0xFFu) - 127.5f;
                    fdz = static_cast<float>((seed >> 16u) & 0xFFu) - 127.5f;

                    r2 = fdx * fdx + fdy * fdy + fdz * fdz;
                }

                const float r2safe = std::max(r2, h * h * 1e-6f);
                const float r = std::sqrt(r2safe);
                const float invR = 1.0f / r;

                const float gradW = CpuSPH::spikyGradCoeff(r, h);
                const float coeff = particles.mass[j] * invRestDensity * gradW;

                const float gx = coeff * dx * invR;
                const float gy = coeff * dy * invR;
                const float gz = coeff * dz * invR;

                sumGrad2 += gx * gx + gy * gy + gz * gz;

                gradCiX -= gx;
                gradCiY -= gy;
                gradCiZ -= gz;
            }

            sumGrad2 +=
                gradCiX * gradCiX +
                gradCiY * gradCiY +
                gradCiZ * gradCiZ;

            particles.lambda[i] = -constraint / (sumGrad2 + epsilon);
        }
    }
}