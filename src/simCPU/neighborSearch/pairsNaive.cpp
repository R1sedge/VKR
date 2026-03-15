#include "pairsNaive.h"


void findPairsNaive(const Particles2D& p, float radius, std::vector<CollisionPair>& out)
{
    const int n = p.count;
    const float minDist = 2.0f * radius;
    const float minDist2 = minDist * minDist;

    for (int i = 0; i < n - 1; ++i)
    {
        for (int j = i + 1; j < n; ++j)
        {
            float dx = p.x[i] - p.x[j];
            float dy = p.y[i] - p.y[j];
            float dist2 = dx * dx + dy * dy;

            if (dist2 < minDist2)
                out.push_back({i, j});
        }
    }
}