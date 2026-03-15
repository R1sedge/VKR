#pragma once

#include "simCUDA/cudaParticles.cuh"

void launchClearDerived(DeviceParticles2D& dp);
void launchPredictPositions(DeviceParticles2D& dp, float dt, float gx, float gy, float velocityDamping);
void launchUpdateVelocities(DeviceParticles2D& dp, float dt);