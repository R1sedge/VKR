#include "simCPU/kernels/cpuKernelsBasic.h"

#include <cmath>

namespace CpuBasicKernels
{
    void clearDerived(Particles3D& particles)
    {
        const int n = particles.count;

        #pragma omp parallel for
        for (int i = 0; i < n; ++i)
        {
            particles.density[i] = 0.0f;
            particles.lambda[i] = 0.0f;
            particles.dx[i] = 0.0f;
            particles.dy[i] = 0.0f;
            particles.dz[i] = 0.0f;
        }
    }

    void predictPositions(
        Particles3D& particles,
        float dt,
        float gx,
        float gy,
        float gz,
        float velocityDamping)
    {
        const int n = particles.count;
        const float damp = 1.0f - velocityDamping;

        #pragma omp parallel for
        for (int i = 0; i < n; ++i)
        {
            // Порядок как в CUDA:
            // сначала damping, затем внешнее ускорение.
            particles.vx[i] *= damp;
            particles.vy[i] *= damp;
            particles.vz[i] *= damp;

            particles.vx[i] += gx * dt;
            particles.vy[i] += gy * dt;
            particles.vz[i] += gz * dt;

            particles.px[i] = particles.x[i];
            particles.py[i] = particles.y[i];
            particles.pz[i] = particles.z[i];

            particles.x[i] += particles.vx[i] * dt;
            particles.y[i] += particles.vy[i] * dt;
            particles.z[i] += particles.vz[i] * dt;
        }
    }

    void updateVelocities(
        Particles3D& particles,
        float dt,
        float maxSpeed)
    {
        if (particles.count <= 0 || dt <= 0.0f)
            return;

        const int n = particles.count;
        const float invDt = 1.0f / dt;
        const float maxSpeedSq = maxSpeed * maxSpeed;

        #pragma omp parallel for
        for (int i = 0; i < n; ++i)
        {
            float newVx = (particles.x[i] - particles.px[i]) * invDt;
            float newVy = (particles.y[i] - particles.py[i]) * invDt;
            float newVz = (particles.z[i] - particles.pz[i]) * invDt;

            const float speedSq =
                newVx * newVx +
                newVy * newVy +
                newVz * newVz;

            if (maxSpeed > 0.0f && speedSq > maxSpeedSq)
            {
                const float invSpeed = maxSpeed / std::sqrt(speedSq);
                newVx *= invSpeed;
                newVy *= invSpeed;
                newVz *= invSpeed;
            }

            particles.vx[i] = newVx;
            particles.vy[i] = newVy;
            particles.vz[i] = newVz;
        }
    }
}