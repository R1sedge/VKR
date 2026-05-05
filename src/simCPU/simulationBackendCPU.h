#pragma once

#include <vector>

#include "common/Config.h"
#include "data/particleData.h"
#include "scene/boundary/boundaryPlane.h"
#include "scene/boundary/vesselBoundary.h"
#include "scene/sceneDescription.h"
#include "sim/simulationBackend.h"
#include "sim/structs.h"
#include "simCPU/constraints/boxBounds.h"
#include "simCPU/neighborSearch/neighborsGrid.h"

class SimulationBackendCPU final : public ISimulationBackendImpl
{
public:
    SimulationBackendCPU();

    void reset() override;
    void update(float dt) override;

    void setWorldBounds(float left, float right, float bottom, float top, float front, float back) override;

    void loadScene(const SceneDescription& desc) override;
    void setVesselOrientation(const glm::quat& orientation) override;

    void setArtificialPressureK(float k) override { m_artPressureK = k; }
    void setVorticityEpsilon(float e) override { m_vorticityEpsilon = e; }
    void setXsphViscosity(float c) override { m_xsphViscosity = c; }

    const Particles3D& getParticles() const override { return m_particles; }

    void setIterations(int iter) { iterations = iter; }
    void configureGrid(float left, float right,
                   float bottom, float top,
                   float front, float back,
                   float cellSize);
    void setVelocityDamping(float d) { m_velocityDamping = d; }

    const std::vector<int>& getNeighborOffsets() const { return neighborOffsets; }
    const std::vector<int>& getNeighborIds() const { return neighborIds; }

private:
    void refreshSceneCaches();
    void refreshCachedKernelConstants();

    void beginStep();
    void predictPositions(float dt);

    void buildBroadphase();
    void buildNeighbors();

    void finalizeVelocities(float dt);

private:
    int iterations = Config::iterations;

    Particles3D m_particles;

    // Legacy CPU bounds. На этапе 4 заменим на CPU projection to vessel planes.
    BoxBoundsConstraint2D m_boxConstraint;

    UniformGrid3D m_grid;

    // CSR список соседей:
    // соседи частицы i лежат в [neighborOffsets[i], neighborOffsets[i + 1]).
    std::vector<int> neighborOffsets;
    std::vector<int> neighborIds;

    VesselBoundary m_vessel;
    std::vector<BoundaryPlane> m_worldPlanesCache;
    std::vector<InternalBoundaryPatch> m_worldInternalPatchesCache;

    AABB m_gridBounds = { -3.0f, 3.0f, -3.0f, 3.0f, -2.0f, 2.0f };

    SceneDescription m_loadedScene;
    bool m_hasLoadedScene = false;

    float m_velocityDamping = 0.0001f;

    float m_artPressureK = Config::artificialPressureK;
    float m_cachedWDeltaQ = 0.0f;

    // Пока не используются. Будут подключены на этапе 5.
    float m_vorticityEpsilon = Config::vorticityEpsilon;
    float m_xsphViscosity = Config::xsphViscosity;
};