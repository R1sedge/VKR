#include "simCPU/pbf/cpuPbfDeltaPositions.h"

#include <algorithm>
#include <cmath>

#include "simCPU/utils/cpuSphKernels.h"

namespace CpuPBF
{
    void computeDeltaPositions(
        Particles3D& particles,
        const std::vector<int>& neighborOffsets,
        const std::vector<int>& neighborIds,
        float restDensity,
        float smoothingRadius,
        float artPressureK,
        float wDeltaQ)
    {
        const int n = particles.count;
        if (n <= 0)
            return;

        const float invRestDensity = 1.0f / restDensity;
        const float h = smoothingRadius;

        #pragma omp parallel for
        for (int i = 0; i < n; ++i)
        {
            const float xi = particles.x[i];
            const float yi = particles.y[i];
            const float zi = particles.z[i];
            const float lambdaI = particles.lambda[i];

            float deltaX = 0.0f;
            float deltaY = 0.0f;
            float deltaZ = 0.0f;

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

                // scorr = -k * (W(r,h) / W(deltaQ*h,h))^4
                const float wij = CpuSPH::poly6(r, h);
                const float ratio = (wDeltaQ > 1e-30f) ? (wij / wDeltaQ) : 0.0f;
                const float ratio2 = ratio * ratio;
                const float scorr = -artPressureK * (ratio2 * ratio2);

                const float gradW = CpuSPH::spikyGradCoeff(r, h);
                const float coeff =
                    (lambdaI + particles.lambda[j] + scorr) *
                    particles.mass[j] *
                    invRestDensity *
                    gradW;

                deltaX += coeff * dx * invR;
                deltaY += coeff * dy * invR;
                deltaZ += coeff * dz * invR;
            }

            particles.dx[i] = deltaX;
            particles.dy[i] = deltaY;
            particles.dz[i] = deltaZ;
        }
    }

    void applyDeltaPositions(Particles3D& particles)
    {
        const int n = particles.count;
        if (n <= 0)
            return;

        #pragma omp parallel for
        for (int i = 0; i < n; ++i)
        {
            particles.x[i] += particles.dx[i];
            particles.y[i] += particles.dy[i];
            particles.z[i] += particles.dz[i];

            particles.dx[i] = 0.0f;
            particles.dy[i] = 0.0f;
            particles.dz[i] = 0.0f;
        }
    }
}