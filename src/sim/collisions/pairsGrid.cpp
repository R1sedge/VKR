#include "pairsGrid.h"

#include "cmath"
#include <algorithm>

int UniformGrid2D::clampCellX(float x) const
{
    int cx = static_cast<int>(std::floor((x - left) / cellSize));
    if (cx < 0) return 0;
    if (cx >= cellsX) return cellsX - 1;
    return cx;
}

int UniformGrid2D::clampCellY(float y) const
{
    int cy = static_cast<int>(std::floor((y - bottom) / cellSize));
    if (cy < 0) return 0;
    if (cy >= cellsY) return cellsY - 1;
    return cy;
}

int UniformGrid2D::cellIndex(int cx, int cy) const
{
    return cy * cellsX + cx;
}

void UniformGrid2D::rebuild(float l, float r, float b, float t, float cs)
{
    left = l;
    right = r;
    bottom = b;
    top = t;

    cellSize = cs;

    cellsX = static_cast<int>(std::floor((right - left) / cellSize)) + 1;
    cellsY = static_cast<int>(std::floor((top - bottom) / cellSize)) + 1;
    totalCells = cellsX * cellsY;

    cellCounts.assign(totalCells, 0);
    cellStarts.assign(totalCells, 0);
    cellEnds.assign(totalCells, 0);
    scatterCursor.assign(totalCells, 0);

    particleCell.clear();
    sortedParticleIds.clear();
}

void UniformGrid2D::build(const Particles2D& p)
{
    int n = p.count;

    particleCell.resize(n);
    sortedParticleIds.resize(n);

    if (totalCells == 0)
        return;

    std::fill(cellCounts.begin(), cellCounts.end(), 0);

    // 1) Считаем, сколько частиц попадает в каждую ячейку
    for (int i = 0; i < n; ++i)
    {
        const int cx = clampCellX(p.x[i]);
        const int cy = clampCellY(p.y[i]);
        const int cell = cellIndex(cx, cy);

        particleCell[i] = cell;
        ++cellCounts[cell];
    }

    // 2) Prefix sum -> диапазоны ячеек в sortedParticleIds
    int offset = 0;
    for (int cell = 0; cell < totalCells; ++cell)
    {
        cellStarts[cell] = offset;
        scatterCursor[cell] = offset;
        offset += cellCounts[cell];
        cellEnds[cell] = offset;
    }

    // 3) Scatter индексов частиц в непрерывный массив
    for (int i = 0; i < n; ++i)
    {
        const int cell = particleCell[i];
        const int dst = scatterCursor[cell]++;
        sortedParticleIds[dst] = i;
    }
}

void UniformGrid2D::findPairs(const Particles2D& p, float radius, std::vector<CollisionPair>& out) const
{
    const float minDist = 2.0f * radius;
    const float minDist2 = minDist * minDist;

    if (totalCells == 0)
        return;

    // half-stencil: текущая ячейка + 4 "вперёд" соседа
    static constexpr int neighborOffsets[4][2] = {
        { 1,  0},
        { 0,  1},
        { 1,  1},
        {-1,  1}
    };

    for (int cy = 0; cy < cellsY; ++cy)
    {
        for (int cx = 0; cx < cellsX; ++cx)
        {
            const int cell = cellIndex(cx, cy);
            const int beginA = cellStarts[cell];
            const int endA = cellEnds[cell];

            // Пары внутри той же ячейки
            for (int a = beginA; a < endA; ++a)
            {
                const int i = sortedParticleIds[a];

                for (int b = a + 1; b < endA; ++b)
                {
                    const int j = sortedParticleIds[b];

                    const float dx = p.x[i] - p.x[j];
                    const float dy = p.y[i] - p.y[j];
                    const float dist2 = dx * dx + dy * dy;

                    if (dist2 < minDist2)
                        out.push_back({i, j});
                }
            }

            // Пары с частью соседних ячеек, чтобы не было дублей
            for (const auto& off : neighborOffsets)
            {
                const int nx = cx + off[0];
                const int ny = cy + off[1];

                if (nx < 0 || nx >= cellsX || ny < 0 || ny >= cellsY)
                    continue;

                const int neighborCell = cellIndex(nx, ny);
                const int beginB = cellStarts[neighborCell];
                const int endB = cellEnds[neighborCell];

                for (int a = beginA; a < endA; ++a)
                {
                    const int i = sortedParticleIds[a];

                    for (int b = beginB; b < endB; ++b)
                    {
                        const int j = sortedParticleIds[b];

                        const float dx = p.x[i] - p.x[j];
                        const float dy = p.y[i] - p.y[j];
                        const float dist2 = dx * dx + dy * dy;

                        if (dist2 < minDist2)
                            out.push_back({i, j});
                    }
                }
            }
        }
    }
}
