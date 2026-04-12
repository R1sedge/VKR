#pragma once

#include <memory>

#include "data/particleData.h"
#include "sim/simulationBackendType.h"
#include "scene/sceneDescription.h"

class ISimulationBackendImpl
{
public:
    virtual ~ISimulationBackendImpl() = default;

    virtual void reset() = 0;
    virtual void update(float dt) = 0;
    virtual void setWorldBounds(float left, float right, float bottom, float top) = 0;

    virtual const Particles2D& getParticles() const = 0;

    // CUDA -> OpenGL interop
    virtual bool setupInterop(unsigned int vbo) { return false; }
    virtual void resetInterop(unsigned int vbo) {}

    // PBF
    virtual void setArtificialPressureK(float k) {}
    virtual void setVorticityEpsilon(float e) {}
    virtual void setXsphViscosity(float c) {}

    // Mouse interaction
    virtual void applyMouseForce(float worldX, float worldY,
                                 float radius, float strength,
                                 int forceType) {}

    // Сцена
    virtual void loadScene(const SceneDescription& desc) = 0;
};

class SimulationBackend
{
public:
    explicit SimulationBackend(SimulationBackendType type = SimulationBackendType::CPU);

    void reset();
    void update(float dt);
    void setWorldBounds(float left, float right, float bottom, float top);

    const Particles2D& getParticles() const;

    SimulationBackendType getType() const { return m_type; }
    ISimulationBackendImpl* getImpl() { return m_impl.get(); }

    bool setupInterop(unsigned int vbo);
    void resetInterop(unsigned int vbo);
    
    void setArtificialPressureK(float k);
    void setVorticityEpsilon(float e);
    void setXsphViscosity(float c);

    void applyMouseForce(float worldX, float worldY,
                        float radius, float strength,
                        int forceType);

    void loadScene(const SceneDescription& desc);

private:
    void createImplementation();

private:
    SimulationBackendType m_type = SimulationBackendType::CPU;
    std::unique_ptr<ISimulationBackendImpl> m_impl;
};
