#pragma once

#include "simCUDA/utils/cudaParticles.cuh"

// Заполняет GPU-буфер instanceData (x, y, phase, speed) прямо из DeviceParticles3D
void launchFillInstanceData(
    const DeviceParticles3D& dp,
    float* d_instanceBuffer);   // указатель на mapped VBO
