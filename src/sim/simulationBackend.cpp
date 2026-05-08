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

void SimulationBackend::switchTo(SimulationBackendType type)
{
    if (m_type == type && m_impl)
        return;

    m_type = type;
    createImplementation();
}

void SimulationBackend::reset()
{
    m_impl->reset();
}

void SimulationBackend::update(float dt)
{
    m_impl->update(dt);
}

void SimulationBackend::setWorldBounds(float left, float right, float bottom, float top, float front, float back)
{
    m_impl->setWorldBounds(left, right, bottom, top, front, back);
}

const Particles3D& SimulationBackend::getParticles() const
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

void SimulationBackend::loadScene(const SceneDescription& desc)
{
    m_impl->loadScene(desc);
}

void SimulationBackend::setVesselOrientation(const glm::quat& orientation)
{
    m_impl->setVesselOrientation(orientation);
}

void SimulationBackend::setIterations(int iter)
{
    m_impl->setIterations(iter);
}

void SimulationBackend::setBenchmarkSkipReadback(bool enabled)
{
    m_impl->setBenchmarkSkipReadback(enabled);
}

FrameTiming SimulationBackend::getLastFrameTiming() const 
{
    return m_impl->getLastFrameTiming();
}
