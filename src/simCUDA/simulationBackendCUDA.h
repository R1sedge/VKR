#pragma once

#include <vector>

#include <glad/glad.h>
#include <cuda_gl_interop.h>

#include "common/Config.h"
#include "sim/simulationBackend.h"
#include "scene/sceneDescription.h"
#include "simCUDA/utils/cudaParticles.cuh"
#include "simCUDA/utils/cudaBoundaryPlane.cuh"
#include "simCUDA/utils/cudaInternalBoundaryStorage.cuh"
#include "simCUDA/neighborSearch/neighborsGrid.cuh"

#include "bench/FrameTiming.h"

class SimulationBackendCUDA final : public ISimulationBackendImpl
{
public:
    SimulationBackendCUDA();
    ~SimulationBackendCUDA() override;

    void reset() override;
    void update(float dt) override;
    void setWorldBounds(float left, float right, float bottom, float top, float front, float back) override;

    void setArtificialPressureK(float k) override { m_artPressureK = k; }
    void setVorticityEpsilon(float e) override { m_vorticityEpsilon = e; }
    void setXsphViscosity(float c) override { m_xsphViscosity = c; }

    const Particles3D& getParticles() const override;

    // Interop
    void setInteropVbo(GLuint vboId);
    void unregisterInterop();
    void fillInteropBuffer();
    bool setupInterop(unsigned int vbo) override;
    void resetInterop(unsigned int vbo) override;

    void loadScene(const SceneDescription& desc) override;
    void setVesselOrientation(const glm::quat& orientation) override;

    void setIterations(int iter) override { iterations = iter; }
    void setBenchmarkSkipReadback(bool enabled) override { m_benchmarkSkipReadback = enabled; }

    FrameTiming getLastFrameTiming() const override { return m_lastTiming; }

private:
    void syncDeviceToHost();

    void setVesselPlanes(const std::vector<BoundaryPlane>& planes);
    void releaseVesselPlanes();
    void uploadInternalPatches(const std::vector<InternalBoundaryPatch>& patches);
    void computeAngularVelocity(float dt);

    void refreshCachedKernelConstants();

private:
    int iterations = Config::iterations;

    bool m_benchmarkSkipReadback = false;

    Particles3D m_particles;
    DeviceParticles3D m_deviceParticles;

    DeviceUniformGrid m_grid;
    DeviceNeighborList m_neighbors;

    cudaGraphicsResource_t m_vboResource = nullptr;

    VesselBoundary m_vessel;
    std::vector<BoundaryPlane> m_worldPlanesCache;
    AABB m_gridBounds = { -3.0f, 3.0f, -3.0f, 3.0f, -2.0f, 2.0f };

    DeviceBoundaryPlane* m_dVesselPlanes = nullptr;
    int m_vesselPlaneCount = 0;

    glm::quat m_prevVesselOrientation = glm::quat(1, 0, 0, 0);
    glm::vec3 m_vesselAngularVelocity = {0.0f, 0.0f, 0.0f};

    SceneDescription m_loadedScene;
    bool m_hasLoadedScene = false;
    
    float m_velocityDamping = 0.0001f;

    float m_artPressureK = Config::artificialPressureK;
    float m_cachedWDeltaQ = 0.0f;

    float m_vorticityEpsilon = Config::vorticityEpsilon;
    float m_xsphViscosity = Config::xsphViscosity;

    // Benchmark
    cudaEvent_t m_evPredictStart, m_evPredictStop;
    cudaEvent_t m_evNeighborStart, m_evNeighborStop;
    cudaEvent_t m_evSolverStart, m_evSolverStop;
    cudaEvent_t m_evVelCorrectStart, m_evVelCorrectStop;
    FrameTiming m_lastTiming;
};