#include "simulationBackendCPU.h"

#include <algorithm>
#include <cmath>
#include <chrono>

#include <glm/gtc/quaternion.hpp>

#include "common/Config.h"
#include "scene/SceneFiller.h"

#include "simCPU/constraints/cpuKernelsBounds.h"
#include "simCPU/kernels/cpuKernelsBasic.h"
#include "simCPU/pbf/cpuPbfDensity.h"
#include "simCPU/pbf/cpuPbfLambda.h"
#include "simCPU/pbf/cpuPbfDeltaPositions.h"
#include "simCPU/pbf/cpuVorticity.h"
#include "simCPU/pbf/cpuXSPH.h"
#include "simCPU/utils/cpuSphKernels.h"

namespace 
{
    using Clock = std::chrono::high_resolution_clock;
    inline double msNow(Clock::time_point t0) 
    {
        return std::chrono::duration<double, std::milli>(Clock::now() - t0).count();
    }
}

SimulationBackendCPU::SimulationBackendCPU()
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

    computeAngularVelocity(dt);

    beginStep();

    // ── Этап 1: Predict ──────────────────────────────────────────────────────
    auto t0 = Clock::now();
    predictPositions(dt);
    m_lastTiming.predictMs = msNow(t0);

    // ── Этап 2: Neighbor Search ───────────────────────────────────────────────
    t0 = Clock::now();
    buildBroadphase(); //TODO Заменить на 1 вызов
    buildNeighbors();
    m_lastTiming.neighborMs = msNow(t0);

    // ── Этап 3: PBF Solver ───────────────────────────────────────────────────
    t0 = Clock::now();
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

        projectConstraints();
    }
    m_lastTiming.solverMs = msNow(t0);

    // ── Этап 4: Velocity Correct ─────────────────────────────────────────────
    t0 = Clock::now();
    finalizeVelocities(dt);

    applyVorticity(dt);
    applyXsph();

    applyVelocityResponse();
    m_lastTiming.velocityCorrectMs = msNow(t0);

    m_lastTiming.totalStepMs = m_lastTiming.predictMs + m_lastTiming.neighborMs + m_lastTiming.solverMs + m_lastTiming.velocityCorrectMs;
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

void SimulationBackendCPU::computeAngularVelocity(float dt)
{
    const glm::quat dq = m_vessel.orientation * glm::inverse(m_prevVesselOrientation);
    const float sinHalfAngle = glm::length(glm::vec3(dq.x, dq.y, dq.z));

    if (sinHalfAngle > 1e-6f && dt > 1e-6f)
    {
        const float angle = 2.0f * std::atan2(sinHalfAngle, dq.w);
        const glm::vec3 axis = glm::vec3(dq.x, dq.y, dq.z) / sinHalfAngle;

        m_vesselAngularVelocity = axis * (angle / dt);
    }
    else
    {
        m_vesselAngularVelocity = glm::vec3(0.0f);
    }

    m_prevVesselOrientation = m_vessel.orientation;
}

void SimulationBackendCPU::projectConstraints()
{
    if (!m_worldPlanesCache.empty())
    {
        CpuBounds::projectToVesselPlanes(
            m_particles,
            m_worldPlanesCache,
            Config::particleRadius);
    }
    else
    {
        CpuBounds::projectBounds(
            m_particles,
            m_gridBounds.xMin,
            m_gridBounds.xMax,
            m_gridBounds.yMin,
            m_gridBounds.yMax,
            m_gridBounds.zMin,
            m_gridBounds.zMax,
            Config::particleRadius);
    }

    CpuBounds::projectToInternalPatches(
        m_particles,
        m_worldInternalPatchesCache,
        Config::particleRadius);
}

void SimulationBackendCPU::applyVelocityResponse()
{
    if (m_worldPlanesCache.empty())
        return;

    const glm::vec3 pivot = m_vessel.pivot;

    CpuBounds::applyBoundaryVelocityResponse(
        m_particles,
        m_worldPlanesCache,
        Config::particleRadius,
        Config::wallRestitution,
        Config::wallFriction,
        m_vesselAngularVelocity,
        pivot);

    CpuBounds::applyInternalBaffleVelocityResponse(
        m_particles,
        m_worldInternalPatchesCache,
        Config::particleRadius,
        Config::wallRestitution,
        Config::wallFriction,
        m_vesselAngularVelocity,
        pivot);
}

void SimulationBackendCPU::applyVorticity(float dt)
{
    if (m_vorticityEpsilon <= 0.0f)
        return;

    CpuVorticity::computeVorticity(
        m_particles,
        neighborOffsets,
        neighborIds,
        Config::smoothingRadius);

    CpuVorticity::applyVorticityConfinement(
        m_particles,
        neighborOffsets,
        neighborIds,
        dt,
        m_vorticityEpsilon,
        Config::smoothingRadius);
}

void SimulationBackendCPU::applyXsph()
{
    CpuXSPH::applyXSPH(
        m_particles,
        neighborOffsets,
        neighborIds,
        m_xsphViscosity,
        Config::smoothingRadius);
}

void SimulationBackendCPU::loadScene(const SceneDescription& desc)
{
    m_loadedScene = desc;
    m_hasLoadedScene = true;

    // 1. Сохраняем геометрию сосуда
    m_vessel = desc.vessel;
    m_prevVesselOrientation = m_vessel.orientation;
    m_vesselAngularVelocity = glm::vec3(0.0f);

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
