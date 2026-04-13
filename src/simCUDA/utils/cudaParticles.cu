#include "cudaParticles.cuh"

#include <cuda_runtime.h>
#include <vector>

#include "simCUDA/utils/cudaCheck.h" 
#include "simCUDA/utils/cudaMemUtils.cuh"

namespace
{
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
    
    CudaMem::allocFloatArray(dp.x, count);
    CudaMem::allocFloatArray(dp.y, count);
    CudaMem::allocFloatArray(dp.z, count);
    CudaMem::allocFloatArray(dp.px, count);
    CudaMem::allocFloatArray(dp.py, count);
    CudaMem::allocFloatArray(dp.pz, count);

    CudaMem::allocFloatArray(dp.vx, count);
    CudaMem::allocFloatArray(dp.vy, count);
    CudaMem::allocFloatArray(dp.vz, count);

    CudaMem::allocFloatArray(dp.mass, count);

    CudaMem::allocFloatArray(dp.density, count);
    CudaMem::allocFloatArray(dp.lambda, count);
    CudaMem::allocFloatArray(dp.dx, count);
    CudaMem::allocFloatArray(dp.dy, count);
    CudaMem::allocFloatArray(dp.dz, count);

    CudaMem::allocFloatArray(dp.omega, count);

    CudaMem::allocIntArray(dp.phase, count);
}

void freeDeviceParticles(DeviceParticles2D& dp)
{
    CudaMem::freeFloatArray(dp.x);
    CudaMem::freeFloatArray(dp.y);
    CudaMem::freeFloatArray(dp.z);
    CudaMem::freeFloatArray(dp.px);
    CudaMem::freeFloatArray(dp.py);
    CudaMem::freeFloatArray(dp.pz);
    CudaMem::freeFloatArray(dp.vx);
    CudaMem::freeFloatArray(dp.vy);
    CudaMem::freeFloatArray(dp.vz);
    CudaMem::freeFloatArray(dp.mass);

    CudaMem::freeFloatArray(dp.density);
    CudaMem::freeFloatArray(dp.lambda);
    CudaMem::freeFloatArray(dp.dx);
    CudaMem::freeFloatArray(dp.dy);
    CudaMem::freeFloatArray(dp.dz);

    CudaMem::freeFloatArray(dp.omega);

    CudaMem::freeIntArray(dp.phase);

    dp.count = 0;
}

void uploadParticlesToDevice(const Particles2D& hp, DeviceParticles2D& dp)
{
    if (dp.count != hp.count)
    {
        freeDeviceParticles(dp); 
        allocateDeviceParticles(dp, hp.count); 
    }
    
    if (hp.count <= 0)
        return;

    copyHostToDevice(dp.x, hp.x, hp.count);
    copyHostToDevice(dp.y, hp.y, hp.count);
    copyHostToDevice(dp.z, hp.z, hp.count);
    copyHostToDevice(dp.px, hp.px, hp.count);
    copyHostToDevice(dp.py, hp.py, hp.count);
    copyHostToDevice(dp.pz, hp.pz, hp.count);

    copyHostToDevice(dp.vx, hp.vx, hp.count);
    copyHostToDevice(dp.vy, hp.vy, hp.count);
    copyHostToDevice(dp.vz, hp.vz, hp.count);

    copyHostToDevice(dp.mass, hp.mass, hp.count);

    copyHostToDevice(dp.density, hp.density, hp.count);
    copyHostToDevice(dp.lambda, hp.lambda, hp.count);
    copyHostToDevice(dp.dx, hp.dx, hp.count);
    copyHostToDevice(dp.dy, hp.dy, hp.count);
    copyHostToDevice(dp.dz, hp.dz, hp.count);
}

void downloadParticlesFromDevice(const DeviceParticles2D& dp, Particles2D& hp)
{
    hp.resize(dp.count);

    if (dp.count <= 0)
        return;
    
    copyDeviceToHost(hp.x, dp.x, dp.count);
    copyDeviceToHost(hp.y, dp.y, dp.count);
    copyDeviceToHost(hp.z, dp.z, dp.count);
    copyDeviceToHost(hp.px, dp.px, dp.count);
    copyDeviceToHost(hp.py, dp.py, dp.count);
    copyDeviceToHost(hp.pz, dp.pz, dp.count);

    copyDeviceToHost(hp.vx, dp.vx, dp.count);
    copyDeviceToHost(hp.vy, dp.vy, dp.count);
    copyDeviceToHost(hp.vz, dp.vz, dp.count);

    copyDeviceToHost(hp.mass, dp.mass, dp.count);

    copyDeviceToHost(hp.density, dp.density, dp.count);
    copyDeviceToHost(hp.lambda, dp.lambda, dp.count);
    copyDeviceToHost(hp.dx, dp.dx, dp.count);
    copyDeviceToHost(hp.dy, dp.dy, dp.count);
    copyDeviceToHost(hp.dz, dp.dz, dp.count);
}
