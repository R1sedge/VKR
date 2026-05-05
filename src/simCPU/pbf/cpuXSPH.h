#pragma once

#include <vector>

#include "data/particleData.h"

namespace CpuXSPH
{
    void applyXSPH(
        Particles3D& particles,
        const std::vector<int>& neighborOffsets,
        const std::vector<int>& neighborIds,
        float xsphViscosity,
        float smoothingRadius);
}