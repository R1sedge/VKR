#pragma once

#include "data/particleData.h"

namespace CpuBasicKernels
{
    void clearDerived(Particles3D& particles);

    void predictPositions(
        Particles3D& particles,
        float dt,
        float gx,
        float gy,
        float gz,
        float velocityDamping);

    void updateVelocities(
        Particles3D& particles,
        float dt,
        float maxSpeed);
}