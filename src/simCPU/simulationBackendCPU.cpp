#include "simulationBackendCPU.h"

#include <algorithm>
#include <cmath>

#include <glm/gtc/quaternion.hpp>

#include "common/Config.h"
#include "scene/SceneFiller.h"

#include "simCPU/kernels/cpuKernelsBasic.h"
#include "simCPU/pbf/cpuPbfDensity.h"
#include "simCPU/pbf/cpuPbfLambda.h"
#include "simCPU/pbf/cpuPbfDeltaPositions.h"
#include "simCPU/utils/cpuSphKernels.h"

SimulationBackendCPU::SimulationBackendCPU()
    : m_boxConstraint(0.0f, 0.0f, 0.0f, 0.0f)
{
    refreshCachedKernelConstants();
    reset();
}

void SimulationBackendCPU::refreshSceneCaches()
{
    m_worldPlanesCache = m_vessel.getWorldPlanes();
    m_worldInternalPatchesCache = m_vessel.getWorldInternalPatches();
}

void SimulationBackendCPU::refreshCachedKernelConstants()
{
    const float dq = Config::artificialPressureDeltaQ * Config::smoothingRadius;
    m_cachedWDeltaQ = CpuSPH::poly6(dq, Config::smoothingRadius);
}

void SimulationBackendCPU::reset()
{
    if (!m_hasLoadedScene)
    {
        m_particles = Particles3D{};

        neighborOffsets.clear();
        neighborIds.clear();

        setWorldBounds(m_gridBounds.xMin, m_gridBounds.xMax,
                       m_gridBounds.yMin, m_gridBounds.yMax,
                       m_gridBounds.zMin, m_gridBounds.zMax);
        return;
    }

    loadScene(m_loadedScene);
}

void SimulationBackendCPU::setWorldBounds(float left, float right,
                                          float bottom, float top,
                                          float front, float back)
{
    m_gridBounds = AABB{
        left, right,
        bottom, top,
        front, back
    };

    m_boxConstraint.setBounds(left, right, bottom, top);

    const float cellSize = Config::smoothingRadius;

    configureGrid(left, right, bottom, top, front, back, cellSize);
}

void SimulationBackendCPU::configureGrid(float left, float right,
                                         float bottom, float top,
                                         float front, float back,
                                         float cellSize)
{
    m_grid.rebuild(left, right, bottom, top, front, back, cellSize);
}

void SimulationBackendCPU::update(float dt)
{
    if (m_particles.count == 0)
        return;

    beginStep();

    predictPositions(dt);

    buildBroadphase();
    buildNeighbors();

    for (int iter = 0; iter < iterations; ++iter)
    {
        CpuPBF::computeDensity(
            m_particles,
            neighborOffsets,
            neighborIds,
            Config::smoothingRadius);

        CpuPBF::computeLambda(
            m_particles,
            neighborOffsets,
            neighborIds,
            Config::restDensity,
            Config::epsilon,
            Config::smoothingRadius);

        CpuPBF::computeDeltaPositions(
            m_particles,
            neighborOffsets,
            neighborIds,
            Config::restDensity,
            Config::smoothingRadius,
            m_artPressureK,
            m_cachedWDeltaQ);

        CpuPBF::applyDeltaPositions(m_particles);

        // Временный legacy fallback.
        // На этапе 4 заменим на projectToVesselPlanes + internal patches.
        m_boxConstraint.project(m_particles);
    }

    finalizeVelocities(dt);
}

void SimulationBackendCPU::beginStep()
{
    CpuBasicKernels::clearDerived(m_particles);
}

void SimulationBackendCPU::predictPositions(float dt)
{
    CpuBasicKernels::predictPositions(
        m_particles,
        dt,
        Config::gravityX,
        Config::gravityY,
        Config::gravityZ,
        m_velocityDamping);
}

void SimulationBackendCPU::finalizeVelocities(float dt)
{
    CpuBasicKernels::updateVelocities(
        m_particles,
        dt,
        Config::maxSpeed);
}

void SimulationBackendCPU::buildBroadphase()
{
    m_grid.build(m_particles);
}

void SimulationBackendCPU::buildNeighbors()
{
    CpuNeighborSearch::buildNeighborList3D(
        m_particles,
        m_grid,
        Config::smoothingRadius,
        Config::particleRadius,
        Config::enableBafflePairFiltering,
        m_worldInternalPatchesCache,
        neighborOffsets,
        neighborIds);
}

void SimulationBackendCPU::loadScene(const SceneDescription& desc)
{
    m_loadedScene = desc;
    m_hasLoadedScene = true;

    // 1. Сохраняем геометрию сосуда
    m_vessel = desc.vessel;

    // 2. Строим частицы из новой scene-модели
    m_particles = SceneFiller::fill(desc);
    m_particles.clearDerived();

    // 3. Вычисляем bounds для neighbour-grid из VesselBoundary
    const float gridMargin = Config::smoothingRadius + Config::particleRadius;
    m_gridBounds = m_vessel.computeGridAABB(gridMargin);

    // 4. Прокидываем новый grid AABB.
    setWorldBounds(m_gridBounds.xMin, m_gridBounds.xMax,
                   m_gridBounds.yMin, m_gridBounds.yMax,
                   m_gridBounds.zMin, m_gridBounds.zMax);

    // 5. Кэшируем мировые плоскости сосуда и внутренние перегородки.
    refreshSceneCaches();
    refreshCachedKernelConstants();

    neighborOffsets.assign(m_particles.count + 1, 0);
    neighborIds.clear();
}

void SimulationBackendCPU::setVesselOrientation(const glm::quat& orientation)
{
    m_vessel.orientation = glm::normalize(orientation);
    refreshSceneCaches();
}
