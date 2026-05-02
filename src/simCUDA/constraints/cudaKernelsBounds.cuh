#pragma once

#include "simCUDA/utils/cudaParticles.cuh"
#include "simCUDA/utils/cudaBoundaryPlane.cuh"
#include "simCUDA/utils/cudaInternalBoundaryStorage.cuh" 

void launchProjectBounds(
    DeviceParticles3D& dp,
    float left,
    float right,
    float bottom,
    float top,
    float front,
    float back,
    float radius);

void launchProjectToVesselPlanes(
    DeviceParticles3D& dp,
    const DeviceBoundaryPlane* planes,
    int planeCount,
    float radius);

void launchApplyBoundaryVelocityResponse(
    DeviceParticles3D& dp,
    const DeviceBoundaryPlane* planes,
    int planeCount,
    float radius,
    float restitution,
    float friction,
    float angVx, float angVy, float angVz,
    float pivotX, float pivotY, float pivotZ);

void launchProjectToInternalPatches(DeviceParticles3D dp, float particleRadius);

void launchApplyInternalBaffleVelocityResponse(
    DeviceParticles3D dp,
    float particleRadius,
    float restitution,
    float friction,
    float angVx, float angVy, float angVz,
    float pivotX, float pivotY, float pivotZ);
