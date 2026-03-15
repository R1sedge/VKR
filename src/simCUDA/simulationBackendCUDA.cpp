#include "simulationBackendCUDA.h"

#include <stdexcept>

void SimulationBackendCUDA::reset()
{
    throw std::runtime_error("CUDA backend is not implemented yet");
}

void SimulationBackendCUDA::update(float)
{
    throw std::runtime_error("CUDA backend is not implemented yet");
}

void SimulationBackendCUDA::setWorldBounds(float, float, float, float)
{
    throw std::runtime_error("CUDA backend is not implemented yet");
}

const Particles2D& SimulationBackendCUDA::getParticles() const
{
    return m_particles;
}
