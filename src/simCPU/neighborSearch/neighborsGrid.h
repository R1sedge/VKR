#pragma once

#include <vector>

#include "data/particleData.h"
#include "scene/boundary/boundaryPlane.h"

struct UniformGrid3D
{
    float left = 0.0f;
    float right = 0.0f;

    float bottom = 0.0f;
    float top = 0.0f;

    float front = 0.0f;
    float back = 0.0f;

    float cellSize = 0.0f;

    int cellsX = 0;
    int cellsY = 0;
    int cellsZ = 0;
    int totalCells = 0;

    // [particleCount] cell id для каждой частицы.
    std::vector<int> particleCell;

    // CSR-представление ячеек.
    std::vector<int> cellCounts;
    std::vector<int> cellStarts;
    std::vector<int> cellEnds;

    // Индексы частиц, сгруппированные по ячейкам.
    std::vector<int> sortedParticleIds;

    // Временный буфер scatter.
    std::vector<int> scatterCursor;

    void rebuild(float l, float r,
                 float b, float t,
                 float f, float bk,
                 float cs);

    void build(const Particles3D& particles);

    int clampCellX(float x) const;
    int clampCellY(float y) const;
    int clampCellZ(float z) const;

    int cellIndex(int cx, int cy, int cz) const;
};

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
        std::vector<int>& neighborIds);
}