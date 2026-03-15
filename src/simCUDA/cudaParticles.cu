#include "cudaParticles.cuh"

#include <cuda_runtime.h>
#include <vector>

#include "simCUDA/cudaCheck.h" 

namespace
{
    void allocFloatArray(float*& ptr, int count)
    {
        CUDA_CHECK(cudaMalloc(&ptr, sizeof(float) * count));
    }

    void freeFloatArray(float*& ptr)
    {
        if (ptr != nullptr)
        {
            CUDA_CHECK(cudaFree(ptr));
            ptr = nullptr;
        }
    }

    void copyHostToDevice(float* dst, const std::vector<float>& src, int count)
    {
        CUDA_CHECK(cudaMemcpy(dst, src.data(), sizeof(float) * count, cudaMemcpyHostToDevice));
    }

    void copyDeviceToHost(std::vector<float>& dst, const float* src, int count)
    {
        CUDA_CHECK(cudaMemcpy(dst.data(), src, sizeof(float) * count, cudaMemcpyDeviceToHost));
    }
}

void allocateDeviceParticles(DeviceParticles2D& dp, int count)
{
    freeDeviceParticles(dp);

    dp.count = count;

    if(count <= 0)
        return;
    
    allocFloatArray(dp.x, count);
    allocFloatArray(dp.y, count);
    allocFloatArray(dp.px, count);
    allocFloatArray(dp.py, count);

    allocFloatArray(dp.vx, count);
    allocFloatArray(dp.vy, count);

    allocFloatArray(dp.mass, count);

    allocFloatArray(dp.density, count);
    allocFloatArray(dp.lambda, count);
    allocFloatArray(dp.dx, count);
    allocFloatArray(dp.dy, count);
}

void freeDeviceParticles(DeviceParticles2D& dp)
{
    freeFloatArray(dp.x);
    freeFloatArray(dp.y);
    freeFloatArray(dp.px);
    freeFloatArray(dp.py);
    freeFloatArray(dp.vx);
    freeFloatArray(dp.vy);
    freeFloatArray(dp.mass);

    freeFloatArray(dp.density);
    freeFloatArray(dp.lambda);
    freeFloatArray(dp.dx);
    freeFloatArray(dp.dy);

    dp.count = 0;
}

void uploadParticlesToDevice(const Particles2D& hp, DeviceParticles2D& dp)
{
    if (dp.count != hp.count)
        allocateDeviceParticles(dp, hp.count);
    
    if (hp.count <= 0)
        return;

    copyHostToDevice(dp.x, hp.x, hp.count);
    copyHostToDevice(dp.y, hp.y, hp.count);
    copyHostToDevice(dp.px, hp.px, hp.count);
    copyHostToDevice(dp.py, hp.py, hp.count);

    copyHostToDevice(dp.vx, hp.vx, hp.count);
    copyHostToDevice(dp.vy, hp.vy, hp.count);

    copyHostToDevice(dp.mass, hp.mass, hp.count);

    copyHostToDevice(dp.density, hp.density, hp.count);
    copyHostToDevice(dp.lambda, hp.lambda, hp.count);
    copyHostToDevice(dp.dx, hp.dx, hp.count);
    copyHostToDevice(dp.dy, hp.dy, hp.count);
}

void downloadParticlesFromDevice(const DeviceParticles2D& dp, Particles2D& hp)
{
    hp.resize(dp.count);

    if (dp.count <= 0)
        return;
    
    copyDeviceToHost(hp.x, dp.x, dp.count);
    copyDeviceToHost(hp.y, dp.y, dp.count);
    copyDeviceToHost(hp.px, dp.px, dp.count);
    copyDeviceToHost(hp.py, dp.py, dp.count);

    copyDeviceToHost(hp.vx, dp.vx, dp.count);
    copyDeviceToHost(hp.vy, dp.vy, dp.count);

    copyDeviceToHost(hp.mass, dp.mass, dp.count);
    
    copyDeviceToHost(hp.density, dp.density, dp.count);
    copyDeviceToHost(hp.lambda, dp.lambda, dp.count);
    copyDeviceToHost(hp.dx, dp.dx, dp.count);
    copyDeviceToHost(hp.dy, dp.dy, dp.count);
}
