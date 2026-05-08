#pragma once

#include <memory>

#include "data/particleData.h"
#include "sim/simulationBackendType.h"
#include "scene/sceneDescription.h"

#include "bench/FrameTiming.h" 

class ISimulationBackendImpl
{
public:
    virtual ~ISimulationBackendImpl() = default;

    virtual void reset() = 0;
    virtual void update(float dt) = 0;
    virtual void setWorldBounds(float left, float right, float bottom, float top, float front, float back) = 0;

    virtual const Particles3D& getParticles() const = 0;

    // CUDA -> OpenGL interop
    virtual bool setupInterop(unsigned int vbo) { return false; }
    virtual void resetInterop(unsigned int vbo) {}

    // PBF
    virtual void setArtificialPressureK(float k) {}
    virtual void setVorticityEpsilon(float e) {}
    virtual void setXsphViscosity(float c) {}

    virtual void setVesselOrientation(const glm::quat& orientation) {}

    virtual void loadScene(const SceneDescription& desc) = 0;

    virtual void setIterations(int iter) {}
    virtual void setBenchmarkSkipReadback(bool enabled) {}

    virtual FrameTiming getLastFrameTiming() const { return {}; }
};

class SimulationBackend
{
public:
    explicit SimulationBackend(SimulationBackendType type = SimulationBackendType::CUDA);

    void switchTo(SimulationBackendType type);

    void reset();
    void update(float dt);

    void setWorldBounds(float left, float right, float bottom, float top, float front, float back);

    const Particles3D& getParticles() const;

    SimulationBackendType getType() const { return m_type; }
    ISimulationBackendImpl* getImpl() { return m_impl.get(); }

    bool setupInterop(unsigned int vbo);
    void resetInterop(unsigned int vbo);

    void setArtificialPressureK(float k);
    void setVorticityEpsilon(float e);
    void setXsphViscosity(float c);

    void setVesselOrientation(const glm::quat& orientation);

    void loadScene(const SceneDescription& desc);

    void setIterations(int iter);
    void setBenchmarkSkipReadback(bool enabled);

    FrameTiming getLastFrameTiming() const;

private:
    void createImplementation();

private:
    SimulationBackendType m_type = SimulationBackendType::CUDA;
    std::unique_ptr<ISimulationBackendImpl> m_impl;
};