#pragma once

#include "simCUDA/utils/cudaParticles.cuh"
#include "simCUDA/utils/cudaBoundaryPlane.cuh"

void launchProjectBounds(
    DeviceParticles3D& dp,
    float left,
    float right,
    float bottom,
    float top,
    float front,
    float back,
    float radius);

void launchProjectToVesselPlanes(DeviceParticles3D& dp,
                                 const DeviceBoundaryPlane* planes,
                                 int planeCount,
                                 float radius);