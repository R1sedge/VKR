#include "circleCollision.h"
#include "cmath"
#include <algorithm>  // std::max

void CircleCollisionConstraint2D::project(Particles2D& particles, std::vector<CollisionPair>& pairs)
{
    const float minDist = 2.0f * radius;
    const float eps = 1e-6f;

    for (const auto& pair: pairs)
    {
        int i = pair.i;
        int j = pair.j;

        float dx = particles.x[i] - particles.x[j];
        float dy = particles.y[i] - particles.y[j];

        float dist = std::max(std::sqrt(dx* dx + dy * dy), eps);

        float overlap = minDist - dist;
        if (overlap <= 0.0f) continue;

        // Единичный вектор от j к i
        float invDist = 1.0f / dist;
        float nx = dx * invDist;
        float ny = dy * invDist;

        // Взвешенная коррекция по массе (PBD: w_i = 1/m_i)
        float mi = particles.mass[i];
        float mj = particles.mass[j];
        float invMassSum = 1.0f / (mi + mj);

        float wi = mj * invMassSum;
        float wj = mi * invMassSum;

        particles.x[i] += wi * overlap * nx;
        particles.y[i] += wi * overlap * ny;
        particles.x[j] -= wj * overlap * nx;
        particles.y[j] -= wj * overlap * ny; 
    }
}