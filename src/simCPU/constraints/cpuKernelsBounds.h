#pragma once

#include <vector>

#include <glm/glm.hpp>

#include "data/particleData.h"
#include "scene/boundary/boundaryPlane.h"

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
        float radius);

    void projectToVesselPlanes(
        Particles3D& particles,
        const std::vector<BoundaryPlane>& planes,
        float radius);

    void projectToInternalPatches(
        Particles3D& particles,
        const std::vector<InternalBoundaryPatch>& internalPatches,
        float particleRadius);

    void applyBoundaryVelocityResponse(
        Particles3D& particles,
        const std::vector<BoundaryPlane>& planes,
        float radius,
        float restitution,
        float friction,
        const glm::vec3& angularVelocity,
        const glm::vec3& pivot);

    void applyInternalBaffleVelocityResponse(
        Particles3D& particles,
        const std::vector<InternalBoundaryPatch>& internalPatches,
        float particleRadius,
        float restitution,
        float friction,
        const glm::vec3& angularVelocity,
        const glm::vec3& pivot);
}