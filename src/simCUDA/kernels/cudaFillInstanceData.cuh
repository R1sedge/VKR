#pragma once

#include "simCUDA/utils/cudaParticles.cuh"

// Заполняет GPU-буфер instanceData (x, y, radius, speed) прямо из DeviceParticles2D
void launchFillInstanceData(
    const DeviceParticles2D& dp,
    float* d_instanceBuffer,   // указатель на mapped VBO
    float radius);
