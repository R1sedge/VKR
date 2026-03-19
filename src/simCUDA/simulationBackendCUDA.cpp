#include "simulationBackendCUDA.h"

#include <cuda_runtime.h>

#include "common/Config.h"
#include "simCUDA/utils/cudaCheck.h"
#include "simCUDA/kernels/cudaKernelsBasic.cuh"
#include "simCUDA/kernels/cudaPbfDensity.cuh"
#include "simCUDA/kernels/cudaParticleCollisionProject.cuh"
#include "simCUDA/kernels/cudaPbfLambda.cuh"
#include "simCUDA/kernels/cudaPbfDeltaPositions.cuh"
#include "simCUDA/constraints/cudaKernelsBounds.cuh"
#include "simCUDA/neighborSearch/neighborsGrid.cuh"
#include "simCUDA/utils/cudaParticles.cuh"
#include "simCUDA/kernels/cudaFillInstanceData.cuh"


SimulationBackendCUDA::SimulationBackendCUDA()
{
    reset();
}

SimulationBackendCUDA::~SimulationBackendCUDA()
{
    freeDeviceCollisionCheck(m_collisionCheck);
    freeDeviceNeighborList(m_neighbors);
    freeDeviceParticles(m_deviceParticles);
}

void SimulationBackendCUDA::setWorldBounds(float left, float right, float bottom, float top)
{
    m_left = left;
    m_right = right;
    m_bottom = bottom;
    m_top = top;
}

void SimulationBackendCUDA::reset()
{
    const float r = Config::particleRadius;
    const float step = r * 2.5f;

    const int cols = 200;
    const int rows = 80;
    const int n = cols * rows;

    m_particles.resize(n);

    for (int row = 0; row < rows; ++row)
    {
        for (int col = 0; col < cols; ++col)
        {
            const int idx = row * cols + col;

            const float fx = (col - cols / 2) * step;
            const float fy = (row - rows / 2) * step;

            m_particles.x[idx] = fx;
            m_particles.y[idx] = fy;
            m_particles.px[idx] = fx;
            m_particles.py[idx] = fy;
            m_particles.vx[idx] = 0.0f;
            m_particles.vy[idx] = 0.0f;
            m_particles.mass[idx] = 1.0f;
        }
    }

    m_particles.clearDerived();
    uploadParticlesToDevice(m_particles, m_deviceParticles);
}

void SimulationBackendCUDA::setInteropVbo(GLuint vboId) 
{
    // Снять старую регистрацию если была
    unregisterInterop();
    CUDA_CHECK(cudaGraphicsGLRegisterBuffer(
        &m_vboResource,
        vboId,
        cudaGraphicsMapFlagsWriteDiscard));  // GPU пишет, GL читает
}

void SimulationBackendCUDA::unregisterInterop() 
{
    if (m_vboResource) 
    {
        cudaGraphicsUnregisterResource(m_vboResource);
        m_vboResource = nullptr;
    }
}

void SimulationBackendCUDA::fillInteropBuffer() 
{
    if (!m_vboResource || m_deviceParticles.count == 0) return;

    float* d_ptr = nullptr;
    size_t sz = 0;

    CUDA_CHECK(cudaGraphicsMapResources(1, &m_vboResource, 0));
    CUDA_CHECK(cudaGraphicsResourceGetMappedPointer(
        (void**)&d_ptr, &sz, m_vboResource));
        
    launchFillInstanceData(m_deviceParticles, d_ptr, Config::particleRadius);
    CUDA_CHECK(cudaGraphicsUnmapResources(1, &m_vboResource, 0));
}

void SimulationBackendCUDA::update(float dt)
{
    if (m_deviceParticles.count <= 0)
        return;

    launchClearDerived(m_deviceParticles);

    launchPredictPositions(
        m_deviceParticles,
        dt,
        Config::gravityX,
        Config::gravityY,
        m_velocityDamping);

    buildNeighborsGridCUDA(
        m_deviceParticles, 
        m_neighbors, 
        m_grid,
        Config::smoothingRadius,
        m_left, 
        m_right, 
        m_bottom,
        m_top);

    
    for (int iter = 0; iter < iterations; ++iter)
    {
        launchCheckParticleCollisions(
            m_deviceParticles, 
            m_neighbors,                     
            m_collisionCheck, 
            Config::particleRadius);

        launchComputeDensity(
            m_deviceParticles,
            m_neighbors,
            Config::smoothingRadius);
        
        launchComputeLambda(
        m_deviceParticles,
        m_neighbors,
        Config::restDensity,
        Config::epsilon,
        Config::smoothingRadius);

        launchComputeDeltaPositions(
            m_deviceParticles,
            m_neighbors,
            Config::restDensity,
            Config::smoothingRadius);

        launchApplyDeltaPositions(
            m_deviceParticles,
            0.005f);
    
        launchProjectParticleCollisions(
            m_deviceParticles,
            m_neighbors,
            Config::particleRadius);

        launchProjectBounds(
            m_deviceParticles,
            m_left,
            m_right,
            m_bottom,
            m_top,
            Config::particleRadius);
    }

    launchUpdateVelocities(m_deviceParticles, dt);

    // ======= CUDA-GL INTEROP: пишем в VBO прямо на GPU =======
    if (m_vboResource) 
    {
        float* d_ptr = nullptr;
        size_t sz = 0;

        CUDA_CHECK(cudaGraphicsMapResources(1, &m_vboResource, 0));
        CUDA_CHECK(cudaGraphicsResourceGetMappedPointer((void**)&d_ptr, &sz, m_vboResource));

        launchFillInstanceData(m_deviceParticles, d_ptr, Config::particleRadius);

        // cudaGraphicsUnmapResources сам синхронизирует стрим перед return
        CUDA_CHECK(cudaGraphicsUnmapResources(1, &m_vboResource, 0));
    } 
    else 
    {
        // Fallback: старый путь без interop
        CUDA_CHECK(cudaDeviceSynchronize());
        syncDeviceToHost();
    }
}

void SimulationBackendCUDA::syncDeviceToHost()
{
    downloadParticlesFromDevice(m_deviceParticles, m_particles);
}

const Particles2D& SimulationBackendCUDA::getParticles() const
{
    return m_particles;
}
