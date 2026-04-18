#pragma once
#include "simCUDA/utils/cudaParticles.cuh"
#include "simCUDA/neighborSearch/deviceNeighborList.cuh"

// Шаг 1: вычислить скалярную завихрённость ω_i -> записывается в particles.omega
void launchComputeVorticity(
    const DeviceParticles3D& particles,
    const DeviceNeighborList& neighbors,
    float smoothingRadius);

// Шаг 2: вычислить вектор локации η, нормировать -> N, применить силу к скорости
void launchApplyVorticityConfinement(
    DeviceParticles3D& particles,
    const DeviceNeighborList& neighbors,
    float dt,
    float vorticityEpsilon,
    float smoothingRadius);   