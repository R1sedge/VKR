#pragma once

#include <vector>

#include "data/particleData.h"

namespace CpuPBF
{
    void computeDeltaPositions(
        Particles3D& particles,
        const std::vector<int>& neighborOffsets,
        const std::vector<int>& neighborIds,
        float restDensity,
        float smoothingRadius,
        float artPressureK,
        float wDeltaQ);

    void applyDeltaPositions(Particles3D& particles);
}