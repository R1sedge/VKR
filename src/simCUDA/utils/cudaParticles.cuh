#pragma once 

#include "data/particleData.h"

struct DeviceParticles2D
{
    int count = 0;

    float* x = nullptr;
    float* y = nullptr;
    float* px = nullptr;
    float* py = nullptr;

    float* vx = nullptr;
    float* vy = nullptr;

    float* mass = nullptr;

    float* density = nullptr;
    float* lambda = nullptr;
    float* dx = nullptr;
    float* dy = nullptr;

    int* phase = nullptr;

    float* omega = nullptr;
};

void allocateDeviceParticles(DeviceParticles2D& dp, int count);
void freeDeviceParticles(DeviceParticles2D& dp);

void uploadParticlesToDevice(const Particles2D& hp, DeviceParticles2D& dp);
void downloadParticlesFromDevice(const DeviceParticles2D& dp, Particles2D& hp);