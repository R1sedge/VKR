#pragma once

#include <vector>

#include "data/particleData.h"

namespace CpuPBF
{
    void computeDensity(
        Particles3D& particles,
        const std::vector<int>& neighborOffsets,
        const std::vector<int>& neighborIds,
        float smoothingRadius);
}