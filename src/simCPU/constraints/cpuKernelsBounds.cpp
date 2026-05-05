#include "simCPU/constraints/cpuKernelsBounds.h"

#include <algorithm>
#include <cmath>

#include "simCPU/utils/cpuInternalBoundaryUtils.h"

namespace
{
    glm::vec3 wallVelocityAt(
        const glm::vec3& position,
        const glm::vec3& angularVelocity,
        const glm::vec3& pivot)
    {
        return glm::cross(angularVelocity, position - pivot);
    }

    glm::vec3 applyVelocityResponse(
        const glm::vec3& velocity,
        const glm::vec3& wallVelocity,
        const glm::vec3& normal,
        float restitution,
        float friction)
    {
        const glm::vec3 relativeVelocity = velocity - wallVelocity;
        const float vn = glm::dot(relativeVelocity, normal);

        // Как в CUDA: реагируем только если частица движется навстречу стенке.
        if (vn >= 0.0f)
            return velocity;

        const glm::vec3 normalVelocity = vn * normal;
        const glm::vec3 tangentVelocity = relativeVelocity - normalVelocity;

        return
            -restitution * normalVelocity +
            (1.0f - friction) * tangentVelocity +
            wallVelocity;
    }
}

namespace CpuBounds
{
    void projectBounds(
        Particles3D& particles,
        float left,
        float right,
        float bottom,
        float top,
        float front,
        float back,
        float radius)
    {
        const int n = particles.count;
        if (n <= 0)
            return;

        #pragma omp parallel for
        for (int i = 0; i < n; ++i)
        {
            particles.x[i] = std::max(left + radius, std::min(right - radius, particles.x[i]));
            particles.y[i] = std::max(bottom + radius, std::min(top - radius, particles.y[i]));
            particles.z[i] = std::max(front + radius, std::min(back - radius, particles.z[i]));
        }
    }

    void projectToVesselPlanes(
        Particles3D& particles,
        const std::vector<BoundaryPlane>& planes,
        float radius)
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

            // CUDA делает 2 прохода по плоскостям.
            for (int pass = 0; pass < 2; ++pass)
            {
                bool any = false;

                for (const BoundaryPlane& plane : planes)
                {
                    const float dist = glm::dot(plane.normal, pos - plane.point);

                    if (dist < radius)
                    {
                        const float correction = radius - dist;
                        pos += correction * plane.normal;
                        any = true;
                    }
                }

                if (!any)
                    break;
            }

            particles.x[i] = pos.x;
            particles.y[i] = pos.y;
            particles.z[i] = pos.z;
        }
    }

    void projectToInternalPatches(
        Particles3D& particles,
        const std::vector<InternalBoundaryPatch>& internalPatches,
        float particleRadius)
    {
        const int n = particles.count;
        if (n <= 0 || internalPatches.empty())
            return;

        #pragma omp parallel for
        for (int i = 0; i < n; ++i)
        {
            glm::vec3 pos(particles.x[i], particles.y[i], particles.z[i]);

            const glm::vec3 prev(particles.px[i], particles.py[i], particles.pz[i]);

            for (const InternalBoundaryPatch& patch : internalPatches)
            {
                const glm::vec3 rel = pos - patch.point;
                const float side = glm::dot(rel, patch.normal);

                if (std::abs(side) >= patch.thickness * 0.5f + particleRadius)
                    continue;

                const float localU = glm::dot(rel, patch.u);
                const float localV = glm::dot(rel, patch.v);

                if (std::abs(localU) > patch.halfWidth)
                    continue;

                if (std::abs(localV) > patch.halfHeight)
                    continue;

                if (CpuInternalBoundaryUtils::insideAperture(localU, localV, patch, particleRadius))
                {
                    continue;
                }

                const float prevSide = glm::dot(prev - patch.point, patch.normal);
                const float sign = (prevSide >= 0.0f) ? 1.0f : -1.0f;

                const float correction = patch.thickness * 0.5f + particleRadius - sign * side;

                pos += correction * patch.normal * sign;
            }

            particles.x[i] = pos.x;
            particles.y[i] = pos.y;
            particles.z[i] = pos.z;
        }
    }

    void applyBoundaryVelocityResponse(
        Particles3D& particles,
        const std::vector<BoundaryPlane>& planes,
        float radius,
        float restitution,
        float friction,
        const glm::vec3& angularVelocity,
        const glm::vec3& pivot)
    {
        const int n = particles.count;
        if (n <= 0 || planes.empty())
            return;

        #pragma omp parallel for
        for (int i = 0; i < n; ++i)
        {
            const glm::vec3 pos(particles.x[i], particles.y[i], particles.z[i]);

            glm::vec3 velocity(particles.vx[i], particles.vy[i], particles.vz[i]);

            for (const BoundaryPlane& plane : planes)
            {
                const float dist = glm::dot(plane.normal, pos - plane.point);

                // CUDA реагирует только вблизи поверхности.
                if (dist > 1.5f * radius)
                    continue;

                const glm::vec3 wallVelocity = wallVelocityAt(pos, angularVelocity, pivot);

                velocity = applyVelocityResponse(velocity, wallVelocity, plane.normal, restitution, friction);
            }

            particles.vx[i] = velocity.x;
            particles.vy[i] = velocity.y;
            particles.vz[i] = velocity.z;
        }
    }

    void applyInternalBaffleVelocityResponse(
        Particles3D& particles,
        const std::vector<InternalBoundaryPatch>& internalPatches,
        float particleRadius,
        float restitution,
        float friction,
        const glm::vec3& angularVelocity,
        const glm::vec3& pivot)
    {
        const int n = particles.count;
        if (n <= 0 || internalPatches.empty())
            return;

        #pragma omp parallel for
        for (int i = 0; i < n; ++i)
        {
            const glm::vec3 pos(
                particles.x[i],
                particles.y[i],
                particles.z[i]);

            glm::vec3 velocity(
                particles.vx[i],
                particles.vy[i],
                particles.vz[i]);

            for (const InternalBoundaryPatch& patch : internalPatches)
            {
                const glm::vec3 rel = pos - patch.point;
                const float dist = glm::dot(rel, patch.normal);

                if (std::abs(dist) > 1.5f * (patch.thickness * 0.5f + particleRadius))
                    continue;

                const float localU = glm::dot(rel, patch.u);
                const float localV = glm::dot(rel, patch.v);

                if (std::abs(localU) > patch.halfWidth)
                    continue;

                if (std::abs(localV) > patch.halfHeight)
                    continue;

                if (CpuInternalBoundaryUtils::insideAperture(
                        localU,
                        localV,
                        patch,
                        particleRadius))
                {
                    continue;
                }

                const glm::vec3 wallVelocity =
                    wallVelocityAt(pos, angularVelocity, pivot);

                velocity = applyVelocityResponse(
                    velocity,
                    wallVelocity,
                    patch.normal,
                    restitution,
                    friction);
            }

            particles.vx[i] = velocity.x;
            particles.vy[i] = velocity.y;
            particles.vz[i] = velocity.z;
        }
    }
}