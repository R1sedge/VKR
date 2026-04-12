#pragma once

#include <glad/glad.h>
#include <cuda_gl_interop.h>

#include "common/Config.h"
#include "sim/simulationBackend.h"
#include "scene/SceneDescription.h"
#include "simCUDA/utils/cudaParticles.cuh"
#include "simCUDA/constraints/collisions/cudaCollisionCheck.cuh"
#include "simCUDA/neighborSearch/neighborsGrid.cuh"

class SimulationBackendCUDA final : public ISimulationBackendImpl
{
public:
    SimulationBackendCUDA();
    ~SimulationBackendCUDA() override;

    void reset() override;
    void update(float dt) override;
    void setWorldBounds(float left, float right, float bottom, float top) override;
    
    void setArtificialPressureK(float k) override { m_artPressureK = k; }
    void setVorticityEpsilon(float e) override { m_vorticityEpsilon = e; }
    void setXsphViscosity(float c) override { m_xsphViscosity = c; }

    void applyMouseForce(float worldX, float worldY,
                        float radius, float strength,
                        int forceType) override;

    const Particles2D& getParticles() const override;

    // Interop
    void setInteropVbo(GLuint vboId);    // вызывается после ensureInstanceBufferSize
    void unregisterInterop();
    void fillInteropBuffer();
    bool setupInterop(unsigned int vbo) override;
    void resetInterop(unsigned int vbo) override;

    void loadScene(const SceneDescription& desc) override;

private:
    void syncDeviceToHost();

private:
    int iterations = Config::iterations;

    Particles2D m_particles;
    DeviceParticles2D m_deviceParticles;

    DeviceUniformGrid m_grid;
    DeviceNeighborList m_neighbors;
    DeviceCollisionCheck m_collisionCheck;
    
    cudaGraphicsResource_t m_vboResource = nullptr;

    float m_left = -3.0f;
    float m_right = 3.0f;
    float m_bottom = -3.0f;
    float m_top = 3.0f;

    float m_velocityDamping = 0.0001f;

    float m_artPressureK = Config::artificialPressureK;
    float m_cachedWDeltaQ = 0.0f; // предвычисляется в setWorldBounds

    float m_vorticityEpsilon = Config::vorticityEpsilon;
    float m_xsphViscosity = Config::xsphViscosity;
};
