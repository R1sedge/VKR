#pragma once

#include "sim/simulationBackend.h"

class SimulationBackendCUDA final : public ISimulationBackendImpl
{
public:
    SimulationBackendCUDA() = default;

    void reset() override;
    void update(float dt) override;
    void setWorldBounds(float left, float right, float bottom, float top) override;

    const Particles2D& getParticles() const override;

private:
    Particles2D m_particles;
};
