#pragma once

#include <vector>

#include "data/particleData.h"

namespace CpuVorticity
{
    void computeVorticity(
        Particles3D& particles,
        const std::vector<int>& neighborOffsets,
        const std::vector<int>& neighborIds,
        float smoothingRadius);

    void applyVorticityConfinement(
        Particles3D& particles,
        const std::vector<int>& neighborOffsets,
        const std::vector<int>& neighborIds,
        float dt,
        float vorticityEpsilon,
        float smoothingRadius);
}