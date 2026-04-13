#pragma once

#include "simCUDA/utils/cudaParticles.cuh"

void launchClearDerived(DeviceParticles2D& dp);
void launchPredictPositions(DeviceParticles2D& dp, float dt, float gx, float gy, float gz, float velocityDamping);
void launchUpdateVelocities(DeviceParticles2D& dp, float dt, float maxSpeed, float radius);