#include "simulationBackendCUDA.h"

#include <cuda_runtime.h>

#include "common/Config.h"
#include "simCUDA/cudaCheck.h"
#include "simCUDA/cudaKernelsBasic.cuh"
#include "simCUDA/cudaPbfDensity.cuh"
#include "simCUDA/constraints/cudaKernelsBounds.cuh"
#include "simCUDA/neighborSearch/neighborsNaive.cuh"
#include "simCUDA/cudaParticles.cuh"



SimulationBackendCUDA::SimulationBackendCUDA()
{
    reset();
}

SimulationBackendCUDA::~SimulationBackendCUDA()
{
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

    const int cols = 50;
    const int rows = 50;
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
            m_particles.vx[idx] = 1.0f;
            m_particles.vy[idx] = 1.0f;
            m_particles.mass[idx] = 1.0f;
        }
    }

    m_particles.clearDerived();
    uploadParticlesToDevice(m_particles, m_deviceParticles);
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

    buildNeighborsNaiveCUDA(
        m_deviceParticles,
        m_neighbors,
        Config::smoothingRadius);

    launchComputeDensity(
        m_deviceParticles,
        m_neighbors,
        Config::smoothingRadius);
    
     launchProjectBounds(
        m_deviceParticles,
        m_left,
        m_right,
        m_bottom,
        m_top,
        Config::particleRadius);

    launchUpdateVelocities(m_deviceParticles, dt);

    CUDA_CHECK(cudaDeviceSynchronize());
    syncDeviceToHost();
}

void SimulationBackendCUDA::syncDeviceToHost()
{
    downloadParticlesFromDevice(m_deviceParticles, m_particles);
}

const Particles2D& SimulationBackendCUDA::getParticles() const
{
    return m_particles;
}
