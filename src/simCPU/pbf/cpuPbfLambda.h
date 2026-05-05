#pragma once

#include <vector>

#include "data/particleData.h"

namespace CpuPBF
{
    void computeLambda(
        Particles3D& particles,
        const std::vector<int>& neighborOffsets,
        const std::vector<int>& neighborIds,
        float restDensity,
        float epsilon,
        float smoothingRadius);
}