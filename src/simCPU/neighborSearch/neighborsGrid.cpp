#include "simCPU/neighborSearch/neighborsGrid.h"

#include <algorithm>
#include <cmath>

#include <glm/glm.hpp>

#include "simCPU/utils/cpuInternalBoundaryUtils.h"

namespace
{
    int clampInt(int v, int lo, int hi)
    {
        return std::max(lo, std::min(v, hi));
    }
}

void UniformGrid3D::rebuild(float l, float r,
                            float b, float t,
                            float f, float bk,
                            float cs)
{
    left = l;
    right = r;

    bottom = b;
    top = t;

    front = f;
    back = bk;

    cellSize = cs;

    if (cellSize <= 0.0f)
    {
        cellsX = 0;
        cellsY = 0;
        cellsZ = 0;
        totalCells = 0;

        particleCell.clear();
        cellCounts.clear();
        cellStarts.clear();
        cellEnds.clear();
        sortedParticleIds.clear();
        scatterCursor.clear();
        return;
    }

    cellsX = std::max(1, static_cast<int>(std::floor((right - left) / cellSize)) + 1);
    cellsY = std::max(1, static_cast<int>(std::floor((top - bottom) / cellSize)) + 1);
    cellsZ = std::max(1, static_cast<int>(std::floor((back - front) / cellSize)) + 1);

    totalCells = cellsX * cellsY * cellsZ;

    cellCounts.assign(totalCells, 0);
    cellStarts.assign(totalCells, 0);
    cellEnds.assign(totalCells, 0);
    scatterCursor.assign(totalCells, 0);

    particleCell.clear();
    sortedParticleIds.clear();
}

int UniformGrid3D::clampCellX(float x) const
{
    const int cx = static_cast<int>(std::floor((x - left) / cellSize));
    return clampInt(cx, 0, cellsX - 1);
}

int UniformGrid3D::clampCellY(float y) const
{
    const int cy = static_cast<int>(std::floor((y - bottom) / cellSize));
    return clampInt(cy, 0, cellsY - 1);
}

int UniformGrid3D::clampCellZ(float z) const
{
    const int cz = static_cast<int>(std::floor((z - front) / cellSize));
    return clampInt(cz, 0, cellsZ - 1);
}

int UniformGrid3D::cellIndex(int cx, int cy, int cz) const
{
    return cz * cellsY * cellsX + cy * cellsX + cx;
}

void UniformGrid3D::build(const Particles3D& particles)
{
    const int n = particles.count;

    particleCell.resize(n);
    sortedParticleIds.resize(n);

    if (n <= 0 || totalCells <= 0 || cellSize <= 0.0f)
        return;

    std::fill(cellCounts.begin(), cellCounts.end(), 0);

    // 1. Каждой частице ставим в соответствие 3D cell id.
    // cellCounts заполняется атомарно, чтобы не было гонок данных.
    #pragma omp parallel for
    for (int i = 0; i < n; ++i)
    {
        const int cx = clampCellX(particles.x[i]);
        const int cy = clampCellY(particles.y[i]);
        const int cz = clampCellZ(particles.z[i]);

        const int cell = cellIndex(cx, cy, cz);
        particleCell[i] = cell;

        #pragma omp atomic update
        cellCounts[cell] += 1;
    }

    // 2. Prefix sum по ячейкам.
    // Оставляем последовательным ради детерминированности и переносимости.
    int offset = 0;
    for (int cell = 0; cell < totalCells; ++cell)
    {
        cellStarts[cell] = offset;
        scatterCursor[cell] = offset;

        offset += cellCounts[cell];

        cellEnds[cell] = offset;
    }

    // 3. Stable scatter: порядок частиц внутри ячейки повторяет порядок particle id.
    // Это полезно для воспроизводимости CPU/CUDA сравнений.
    for (int i = 0; i < n; ++i)
    {
        const int cell = particleCell[i];
        const int dst = scatterCursor[cell]++;

        sortedParticleIds[dst] = i;
    }
}

namespace CpuNeighborSearch
{
    void buildNeighborList3D(
        const Particles3D& particles,
        const UniformGrid3D& grid,
        float smoothingRadius,
        float particleRadius,
        bool baffleFilterEnabled,
        const std::vector<InternalBoundaryPatch>& internalPatches,
        std::vector<int>& neighborOffsets,
        std::vector<int>& neighborIds)
    {
        const int n = particles.count;

        neighborOffsets.assign(n + 1, 0);
        neighborIds.clear();

        if (n <= 0 || smoothingRadius <= 0.0f)
            return;

        if (grid.totalCells <= 0 || grid.cellSize <= 0.0f)
            return;

        const float h = smoothingRadius;
        const float h2 = h * h;

        const int searchRadiusCells =
            std::max(1, static_cast<int>(std::ceil(h / grid.cellSize)));

        const bool useBaffleFilter =
            baffleFilterEnabled && !internalPatches.empty();

        std::vector<int> neighborCounts(n, 0);

        // Pass 1: count.
        #pragma omp parallel for
        for (int i = 0; i < n; ++i)
        {
            const float xi = particles.x[i];
            const float yi = particles.y[i];
            const float zi = particles.z[i];

            const int cx = grid.clampCellX(xi);
            const int cy = grid.clampCellY(yi);
            const int cz = grid.clampCellZ(zi);

            const int minCx = std::max(0, cx - searchRadiusCells);
            const int maxCx = std::min(grid.cellsX - 1, cx + searchRadiusCells);

            const int minCy = std::max(0, cy - searchRadiusCells);
            const int maxCy = std::min(grid.cellsY - 1, cy + searchRadiusCells);

            const int minCz = std::max(0, cz - searchRadiusCells);
            const int maxCz = std::min(grid.cellsZ - 1, cz + searchRadiusCells);

            int count = 0;

            for (int nz = minCz; nz <= maxCz; ++nz)
            {
                for (int ny = minCy; ny <= maxCy; ++ny)
                {
                    for (int nx = minCx; nx <= maxCx; ++nx)
                    {
                        const int cell = grid.cellIndex(nx, ny, nz);
                        const int begin = grid.cellStarts[cell];
                        const int end = grid.cellEnds[cell];

                        for (int k = begin; k < end; ++k)
                        {
                            const int j = grid.sortedParticleIds[k];
                            if (j == i)
                                continue;

                            const float dx = xi - particles.x[j];
                            const float dy = yi - particles.y[j];
                            const float dz = zi - particles.z[j];

                            if (dx * dx + dy * dy + dz * dz >= h2)
                                continue;

                            if (useBaffleFilter)
                            {
                                const glm::vec3 a(xi, yi, zi);
                                const glm::vec3 b(
                                    particles.x[j],
                                    particles.y[j],
                                    particles.z[j]);

                                if (CpuInternalBoundaryUtils::segmentBlockedByAnyInternalPatch(
                                        a, b, internalPatches, particleRadius))
                                {
                                    continue;
                                }
                            }

                            ++count;
                        }
                    }
                }
            }

            neighborCounts[i] = count;
        }

        // Prefix sum по количествам соседей.
        int offset = 0;
        for (int i = 0; i < n; ++i)
        {
            neighborOffsets[i] = offset;
            offset += neighborCounts[i];
        }
        neighborOffsets[n] = offset;

        neighborIds.resize(offset);

        if (offset == 0)
            return;

        // Pass 2: fill.
        #pragma omp parallel for
        for (int i = 0; i < n; ++i)
        {
            const float xi = particles.x[i];
            const float yi = particles.y[i];
            const float zi = particles.z[i];

            const int cx = grid.clampCellX(xi);
            const int cy = grid.clampCellY(yi);
            const int cz = grid.clampCellZ(zi);

            const int minCx = std::max(0, cx - searchRadiusCells);
            const int maxCx = std::min(grid.cellsX - 1, cx + searchRadiusCells);

            const int minCy = std::max(0, cy - searchRadiusCells);
            const int maxCy = std::min(grid.cellsY - 1, cy + searchRadiusCells);

            const int minCz = std::max(0, cz - searchRadiusCells);
            const int maxCz = std::min(grid.cellsZ - 1, cz + searchRadiusCells);

            int write = neighborOffsets[i];

            for (int nz = minCz; nz <= maxCz; ++nz)
            {
                for (int ny = minCy; ny <= maxCy; ++ny)
                {
                    for (int nx = minCx; nx <= maxCx; ++nx)
                    {
                        const int cell = grid.cellIndex(nx, ny, nz);
                        const int begin = grid.cellStarts[cell];
                        const int end = grid.cellEnds[cell];

                        for (int k = begin; k < end; ++k)
                        {
                            const int j = grid.sortedParticleIds[k];
                            if (j == i)
                                continue;

                            const float dx = xi - particles.x[j];
                            const float dy = yi - particles.y[j];
                            const float dz = zi - particles.z[j];

                            if (dx * dx + dy * dy + dz * dz >= h2)
                                continue;

                            if (useBaffleFilter)
                            {
                                const glm::vec3 a(xi, yi, zi);
                                const glm::vec3 b(
                                    particles.x[j],
                                    particles.y[j],
                                    particles.z[j]);

                                if (CpuInternalBoundaryUtils::segmentBlockedByAnyInternalPatch(
                                        a, b, internalPatches, particleRadius))
                                {
                                    continue;
                                }
                            }

                            neighborIds[write++] = j;
                        }
                    }
                }
            }
        }
    }
}