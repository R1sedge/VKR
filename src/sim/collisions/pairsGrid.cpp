#include "pairsGrid.h"
#include "cmath"
#include <algorithm>

void UniformGrid2D::rebuild(float l, float r, float b, float t, float cs)
{
    left = l;
    right = r;
    bottom = b;
    top = t;

    cellSize = cs;

    cellsX = int(std::floor((r - l)  / cs)) + 1;
    cellsY = int(std::floor((t - b)  / cs)) + 1;

    grid.assign(cellsY, std::vector<std::vector<int>>(cellsX));
}

void UniformGrid2D::build(const Particles2D& p)
{
    for (int y = 0; y < cellsY; ++y)
        for (int x = 0; x < cellsX; ++x)
        grid[y][x].clear();

    for(int i = 0; i < p.count; ++i)
    {
        int xid = int(std::floor((p.x[i] - left) / cellSize));
        int yid = int(std::floor((p.y[i] - bottom) / cellSize));

        if (xid < 0) xid = 0;
        else if (xid >= cellsX) xid = cellsX - 1;

        if (yid < 0) yid = 0;
        else if (yid >= cellsY) yid = cellsY - 1;

        grid[yid][xid].push_back(i);
    }
}

void UniformGrid2D::findPairs(const Particles2D& p, float radius, std::vector<CollisionPair>& out) const
{
    const float minDist = 2.0f * radius;
    const float minDist2 = minDist * minDist;

    for (int cy = 0; cy < cellsY; ++cy)
    for (int cx = 0; cx < cellsX; ++cx)
    {
        for (int idx1 : grid[cy][cx])
        {
            for (int sy = -1; sy <= 1; ++sy)
            for (int sx = -1; sx <= 1; ++sx)
            {
                int ny = cy + sy;
                int nx = cx + sx;
                if (nx < 0 || nx >= cellsX || ny < 0 || ny >= cellsY)
                    continue;

                for (int idx2 : grid[ny][nx])
                {
                    if (idx2 <= idx1) continue; // чтобы не было дублей и self-pair

                    float dx = p.x[idx1] - p.x[idx2];
                    float dy = p.y[idx1] - p.y[idx2];
                    float dist2 = dx*dx + dy*dy;
                    if (dist2 < minDist2)
                        out.push_back({idx1, idx2});
                }
            }
        }
    }

}
