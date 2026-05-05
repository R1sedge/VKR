#pragma once

#include <vector>

#include "data/particleData.h"
#include "scene/boundary/boundaryPlane.h"

namespace CpuBounds
{
    void projectBounds(
        Particles3D& particles,
        float xMin, float xMax,
        float yMin, float yMax,
        float zMin, float zMax,
        float particleRadius);

    void projectToVesselPlanes(
        Particles3D& particles,
        const std::vector<BoundaryPlane>& planes,
        float particleRadius);

    void projectToInternalPatches(
        Particles3D& particles,
        const std::vector<InternalBoundaryPatch>& patches,
        float particleRadius);
}