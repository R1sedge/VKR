#pragma once

#include "simCUDA/utils/cudaParticles.cuh"

void launchClearDerived(DeviceParticles3D& dp);
void launchPredictPositions(DeviceParticles3D& dp, float dt, float gx, float gy, float gz, float velocityDamping);
void launchUpdateVelocities(DeviceParticles3D& dp, float dt, float maxSpeed, float radius);