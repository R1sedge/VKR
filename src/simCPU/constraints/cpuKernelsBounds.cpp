#include "simCPU/constraints/cpuKernelsBounds.h"

#include <algorithm>
#include <cmath>

#include <glm/glm.hpp>

#include "simCPU/utils/cpuInternalBoundaryUtils.h"

namespace
{
    float clampToRange(float value, float minValue, float maxValue)
    {
        if (minValue > maxValue)
            return 0.5f * (minValue + maxValue);

        return std::max(minValue, std::min(value, maxValue));
    }
}

namespace CpuBounds
{
    void projectBounds(
        Particles3D& particles,
        float xMin, float xMax,
        float yMin, float yMax,
        float zMin, float zMax,
        float particleRadius)
    {
        const int n = particles.count;
        if (n <= 0)
            return;

        const float minX = xMin + particleRadius;
        const float maxX = xMax - particleRadius;

        const float minY = yMin + particleRadius;
        const float maxY = yMax - particleRadius;

        const float minZ = zMin + particleRadius;
        const float maxZ = zMax - particleRadius;

        #pragma omp parallel for
        for (int i = 0; i < n; ++i)
        {
            particles.x[i] = clampToRange(particles.x[i], minX, maxX);
            particles.y[i] = clampToRange(particles.y[i], minY, maxY);
            particles.z[i] = clampToRange(particles.z[i], minZ, maxZ);
        }
    }

    void projectToVesselPlanes(
        Particles3D& particles,
        const std::vector<BoundaryPlane>& planes,
        float particleRadius)
    {
        const int n = particles.count;
        if (n <= 0 || planes.empty())
            return;

        #pragma omp parallel for
        for (int i = 0; i < n; ++i)
        {
            glm::vec3 pos(
                particles.x[i],
                particles.y[i],
                particles.z[i]);

            for (const BoundaryPlane& plane : planes)
            {
                const float dist = glm::dot(pos - plane.point, plane.normal);

                if (dist < particleRadius)
                    pos += plane.normal * (particleRadius - dist);
            }

            particles.x[i] = pos.x;
            particles.y[i] = pos.y;
            particles.z[i] = pos.z;
        }
    }

    void projectToInternalPatches(
        Particles3D& particles,
        const std::vector<InternalBoundaryPatch>& patches,
        float particleRadius)
    {
        const int n = particles.count;
        if (n <= 0 || patches.empty())
            return;

        #pragma omp parallel for
        for (int i = 0; i < n; ++i)
        {
            glm::vec3 pos(
                particles.x[i],
                particles.y[i],
                particles.z[i]);

            const glm::vec3 prevPos(
                particles.px[i],
                particles.py[i],
                particles.pz[i]);

            for (const InternalBoundaryPatch& patch : patches)
            {
                const glm::vec3 rel = pos - patch.point;

                const float localU = glm::dot(rel, patch.u);
                const float localV = glm::dot(rel, patch.v);

                if (std::abs(localU) > patch.halfWidth + particleRadius)
                    continue;

                if (std::abs(localV) > patch.halfHeight + particleRadius)
                    continue;

                if (CpuInternalBoundaryUtils::insideAperture(
                        localU,
                        localV,
                        patch,
                        particleRadius))
                {
                    continue;
                }

                const float side = glm::dot(rel, patch.normal);
                const float allowedDistance = 0.5f * patch.thickness + particleRadius;

                if (std::abs(side) >= allowedDistance)
                    continue;

                const float prevSide = glm::dot(prevPos - patch.point, patch.normal);
                const float sign = (prevSide >= 0.0f) ? 1.0f : -1.0f;

                const float correction = allowedDistance - sign * side;
                pos += sign * correction * patch.normal;
            }

            particles.x[i] = pos.x;
            particles.y[i] = pos.y;
            particles.z[i] = pos.z;
        }
    }
}