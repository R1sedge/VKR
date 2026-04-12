#include "simulationBackend.h"

#include <memory>
#include <stdexcept>

#include "simCPU/simulationBackendCPU.h"
#include "simCUDA/simulationBackendCUDA.h"

SimulationBackend::SimulationBackend(SimulationBackendType type): m_type(type)
{
    createImplementation();
}

void SimulationBackend::createImplementation()
{
    switch (m_type)
    {
    case SimulationBackendType::CPU:
        m_impl = std::make_unique<SimulationBackendCPU>();
        break;

    case SimulationBackendType::CUDA:
        m_impl = std::make_unique<SimulationBackendCUDA>();
        break;

    default:
        throw std::runtime_error("Unknown simulation backend type");
    }
}

void SimulationBackend::reset()
{
    m_impl->reset();
}

void SimulationBackend::update(float dt)
{
    m_impl->update(dt);
}

void SimulationBackend::setWorldBounds(float left, float right, float bottom, float top)
{
    m_impl->setWorldBounds(left, right, bottom, top);
}

const Particles2D& SimulationBackend::getParticles() const
{
    return m_impl->getParticles();
}

bool SimulationBackend::setupInterop(unsigned int vbo)
{
    return m_impl->setupInterop(vbo);
}

void SimulationBackend::resetInterop(unsigned int vbo)
{
    m_impl->resetInterop(vbo);
}

void SimulationBackend::setArtificialPressureK(float k)
{
    m_impl->setArtificialPressureK(k);
}

void SimulationBackend::setVorticityEpsilon(float e) 
{ 
    m_impl->setVorticityEpsilon(e); 
}

void SimulationBackend::setXsphViscosity(float v)
{
    m_impl->setXsphViscosity(v);
}

void SimulationBackend::applyMouseForce(float worldX, float worldY,
                                        float radius, float strength,
                                        int forceType)
{
    if (m_impl)
        m_impl->applyMouseForce(worldX, worldY, radius, strength, forceType);
}

void SimulationBackend::loadScene(const SceneDescription& desc)
{
    m_impl->loadScene(desc);
}