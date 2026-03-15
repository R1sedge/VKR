#pragma once
#include "vector"

#include "data/particleData.h"
#include "sim/structs.h"

struct UniformGrid2D 
{
    float left, right, bottom, top;

    float cellSize;

    int cellsX, cellsY;
    int totalCells;

    std::vector<int> particleCell; // Массив номеров ячеек для частиц

    // CSR Подобное представление ячеек
    std::vector<int> cellCounts;
    std::vector<int> cellStarts;
    std::vector<int> cellEnds;

    // Линейно отсортированные индексы частиц по ячейкам
    std::vector<int> sortedParticleIds;

    // Временный буффер для scatter на этапе build()
    std::vector<int> scatterCursor;

    void rebuild(float l, float r, float b, float t, float cs);
    void build(const Particles2D& p);
    void findPairs(const Particles2D& p, float radius, std::vector<CollisionPair>& out) const;

private:
    int clampCellX(float x) const;
    int clampCellY(float y) const;
    int cellIndex(int cx, int cy) const;
};