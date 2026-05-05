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

    // В CUDA grid cell size = smoothingRadius.
    // Пока CPU еще использует legacy UniformGrid2D, но bounds/cellSize уже синхронизированы
    // с CUDA-подходом; полноценный 3D grid будет на этапе 2.
    const float cellSize = Config::smoothingRadius;
    configureGrid(left, right, bottom, top, cellSize);
}

void SimulationBackendCPU::configureGrid(float left, float right, float bottom, float top, float cellSize)
{
    m_grid.rebuild(left, right, bottom, top, cellSize);
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
    const int n = m_particles.count;
    neighborOffsets.assign(n + 1, 0);
    neighborIds.clear();

    if (n == 0)
        return;

    const float h = Config::smoothingRadius;
    const float h2 = h * h;

    if (m_grid.totalCells == 0 || m_grid.cellSize <= 0.0f)
        return;

    const int searchRadiusCells =
        std::max(1, static_cast<int>(std::ceil(h / m_grid.cellSize)));

    std::vector<int> counts(n, 0);

    for (int i = 0; i < n; ++i)
    {
        const int cell = m_grid.particleCell[i];
        const int cx = cell % m_grid.cellsX;
        const int cy = cell / m_grid.cellsX;

        int count = 0;

        const int minCy = std::max(0, cy - searchRadiusCells);
        const int maxCy = std::min(m_grid.cellsY - 1, cy + searchRadiusCells);
        const int minCx = std::max(0, cx - searchRadiusCells);
        const int maxCx = std::min(m_grid.cellsX - 1, cx + searchRadiusCells);

        for (int ny = minCy; ny <= maxCy; ++ny)
        {
            for (int nx = minCx; nx <= maxCx; ++nx)
            {
                const int neighborCell = ny * m_grid.cellsX + nx;
                const int begin = m_grid.cellStarts[neighborCell];
                const int end = m_grid.cellEnds[neighborCell];

                for (int k = begin; k < end; ++k)
                {
                    const int j = m_grid.sortedParticleIds[k];
                    if (j == i)
                        continue;

                    const float dx = m_particles.x[i] - m_particles.x[j];
                    const float dy = m_particles.y[i] - m_particles.y[j];
                    const float dz = m_particles.z[i] - m_particles.z[j];

                    const float dist2 = dx * dx + dy * dy + dz * dz;

                    if (dist2 < h2)
                        ++count;
                }
            }
        }

        counts[i] = count;
    }

    int offset = 0;
    for (int i = 0; i < n; ++i)
    {
        neighborOffsets[i] = offset;
        offset += counts[i];
    }
    neighborOffsets[n] = offset;

    neighborIds.resize(offset);
    std::vector<int> cursor = neighborOffsets;

    for (int i = 0; i < n; ++i)
    {
        const int cell = m_grid.particleCell[i];
        const int cx = cell % m_grid.cellsX;
        const int cy = cell / m_grid.cellsX;

        const int minCy = std::max(0, cy - searchRadiusCells);
        const int maxCy = std::min(m_grid.cellsY - 1, cy + searchRadiusCells);
        const int minCx = std::max(0, cx - searchRadiusCells);
        const int maxCx = std::min(m_grid.cellsX - 1, cx + searchRadiusCells);

        for (int ny = minCy; ny <= maxCy; ++ny)
        {
            for (int nx = minCx; nx <= maxCx; ++nx)
            {
                const int neighborCell = ny * m_grid.cellsX + nx;
                const int begin = m_grid.cellStarts[neighborCell];
                const int end = m_grid.cellEnds[neighborCell];

                for (int k = begin; k < end; ++k)
                {
                    const int j = m_grid.sortedParticleIds[k];
                    if (j == i)
                        continue;

                    const float dx = m_particles.x[i] - m_particles.x[j];
                    const float dy = m_particles.y[i] - m_particles.y[j];
                    const float dz = m_particles.z[i] - m_particles.z[j];

                    const float dist2 = dx * dx + dy * dy + dz * dz;

                    if (dist2 < h2)
                        neighborIds[cursor[i]++] = j;
                }
            }
        }
    }
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
    // Пока CPU grid legacy-2D, поэтому внутри setWorldBounds используются X/Y bounds,
    // но полный 3D AABB уже сохранен в m_gridBounds для следующего этапа.
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
