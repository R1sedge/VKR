#pragma once

#include "common/Config.h"
#include "sim/simulationBackend.h"
#include "simCUDA/utils/cudaParticles.cuh"
#include "simCUDA/kernels/cudaCollisionCheck.cuh"
#include "simCUDA/neighborSearch/neighborsGrid.cuh"

class SimulationBackendCUDA final : public ISimulationBackendImpl
{
public:
    SimulationBackendCUDA();
    ~SimulationBackendCUDA() override;

    void reset() override;
    void update(float dt) override;
    void setWorldBounds(float left, float right, float bottom, float top) override;

    const Particles2D& getParticles() const override;

private:
    void syncDeviceToHost();

private:
    int iterations = Config::iterations;

    Particles2D m_particles;
    DeviceParticles2D m_deviceParticles;

    DeviceUniformGrid m_grid;
    DeviceNeighborList m_neighbors;
    DeviceCollisionCheck m_collisionCheck;
    

    float m_left = -3.0f;
    float m_right = 3.0f;
    float m_bottom = -3.0f;
    float m_top = 3.0f;

    float m_velocityDamping = 0.001f;
};
