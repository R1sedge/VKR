#pragma once

#include "sim/simulationBackend.h"
#include "simCUDA/cudaParticles.cuh"

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

    float m_left = -1.0f;
    float m_right = 1.0f;
    float m_bottom = -1.0f;
    float m_top = 1.0f;

    float m_velocityDamping = 0.005f;
};
