#pragma once

#include "sim/simulationBackend.h"
#include "simCUDA/cudaParticles.cuh"
#include "simCUDA/neighborSearch/neighborsNaive.cuh"

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
    Particles2D m_particles;
    DeviceParticles2D m_deviceParticles;
    DeviceNeighborList m_neighbors;

    float m_left = -3.0f;
    float m_right = 3.0f;
    float m_bottom = -3.0f;
    float m_top = 3.0f;

    float m_velocityDamping = 0.001f;
};
