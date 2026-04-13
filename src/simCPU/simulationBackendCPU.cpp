#include "simulationBackendCPU.h"

#include "simCPU/neighborSearch/pairsNaive.h"
#include "simCPU/fluid/sphKernels.h"
#include "scene/SceneFiller.h"

#include <algorithm>
#include <cmath>

SimulationBackendCPU::SimulationBackendCPU()
    : boxConstraint(0.0f, 0.0f, 0.0f, 0.0f),
      circleCollision(Config::particleRadius)
{
    reset();
}

void SimulationBackendCPU::reset()
{
    const float r = Config::particleRadius;
    const float step = r * 2.5f;

    const int cols = 50;
    const int rows = 50;
    const int n = cols * rows;

    particles.resize(n);
    collisionPairs.reserve(n * 16); // Верхняя оценка числа пар
    collisionPairs.clear();

    for (int row = 0; row < rows; ++row)
    {
        for (int col = 0; col < cols; ++col)
        {
            int idx = row * cols + col;

            float fx = (col - cols / 2) * step;
            float fy = (row - rows / 2) * step;

            particles.x[idx] = fx;
            particles.y[idx] = fy;
            particles.px[idx] = fx;
            particles.py[idx] = fy;
            particles.vx[idx] = 0.0f;
            particles.vy[idx] = 0.0f;
            particles.mass[idx] = 1.0f;
        }
    }

    particles.clearDerived();
}

void SimulationBackendCPU::setWorldBounds(float left, float right, float bottom, float top)
{
    boxConstraint.setBounds(left, right, bottom, top);

    const float cellSize = 2.0f * Config::particleRadius;
    configureGrid(left, right, bottom, top, cellSize);
}

void SimulationBackendCPU::configureGrid(float left, float right, float bottom, float top, float cellSize)
{
    grid.rebuild(left, right, bottom, top, cellSize);
}

void SimulationBackendCPU::update(float dt)
{
    if (particles.count == 0)
        return;

    beginStep();
    predictPositions(dt);

    buildBroadphase();
    buildNeighbors();
    buildCollisionPairs();

    for (int iter = 0; iter < iterations; ++iter)
    {
        computeDensity();
        computeLambda();
        computeDeltaPositions();
        applyDeltaPositions();

        circleCollision.project(particles, collisionPairs);
        boxConstraint.project(particles); // Чтобы все частицы оставались внутри границ
    }

    finalizeVelocities(dt); // Обновление скоростей
}

void SimulationBackendCPU::beginStep()
{
    particles.clearDerived();
}

void SimulationBackendCPU::predictPositions(float dt)
{
    const float gx = Config::gravityX;
    const float gy = Config::gravityY;
    const float gz = Config::gravityZ;
    const float damp = 1.0f - velocityDamping;

    for (int i = 0; i < particles.count; ++i)
    {
        particles.vx[i] += gx * dt;
        particles.vy[i] += gy * dt;
        particles.vz[i] += gz * dt;

        particles.vx[i] *= damp;
        particles.vy[i] *= damp;
        particles.vz[i] *= damp;

        particles.px[i] = particles.x[i];
        particles.py[i] = particles.y[i];
        particles.pz[i] = particles.z[i];

        particles.x[i] += particles.vx[i] * dt;
        particles.y[i] += particles.vy[i] * dt;
        particles.z[i] += particles.vz[i] * dt;
    }
}

void SimulationBackendCPU::buildBroadphase()
{
    grid.build(particles);
}

void SimulationBackendCPU::buildCollisionPairs()
{
    collisionPairs.clear();
    grid.findPairs(particles, Config::particleRadius, collisionPairs);
}

void SimulationBackendCPU::buildNeighbors()
{
    const int n = particles.count;
    neighborOffsets.assign(n + 1, 0);
    neighborIds.clear();

    if (n == 0)
        return;
    
    const float h = Config::smoothingRadius;
    const float h2 = h * h;

    if (grid.totalCells == 0 || grid.cellSize <= 0.0f)
        return;

    const int searchRadiusCells = std::max(1, static_cast<int>(std::ceil(h / grid.cellSize)));

    std::vector<int> counts(n, 0);

    for (int i = 0; i < n; ++i)
    {
        const int cell = grid.particleCell[i];
        const int cx = cell % grid.cellsX;
        const int cy = cell / grid.cellsX;

        int count = 0;

        const int minCy = std::max(0, cy - searchRadiusCells);
        const int maxCy = std::min(grid.cellsY - 1, cy + searchRadiusCells);
        const int minCx = std::max(0, cx - searchRadiusCells);
        const int maxCx = std::min(grid.cellsX - 1, cx + searchRadiusCells);

        for (int ny = minCy; ny <= maxCy; ++ny)
        {
            for (int nx = minCx; nx <= maxCx; ++nx)
            {
                const int neighborCell = ny * grid.cellsX + nx;
                const int begin = grid.cellStarts[neighborCell];
                const int end   = grid.cellEnds[neighborCell];

                for (int k = begin; k < end; ++k)
                {
                    const int j = grid.sortedParticleIds[k];
                    if (j == i) continue;

                    const float dx = particles.x[i] - particles.x[j];
                    const float dy = particles.y[i] - particles.y[j];
                    const float dist2 = dx * dx + dy * dy;

                    if (dist2 < h2)
                        ++count;
                }
            }
        }
        counts[i] = count;
    }

    // Preffix-sum 
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
        const int cell = grid.particleCell[i];
        const int cx = cell % grid.cellsX;
        const int cy = cell / grid.cellsX;

        const int minCy = std::max(0, cy - searchRadiusCells);
        const int maxCy = std::min(grid.cellsY - 1, cy + searchRadiusCells);
        const int minCx = std::max(0, cx - searchRadiusCells);
        const int maxCx = std::min(grid.cellsX - 1, cx + searchRadiusCells);

        for (int ny = minCy; ny <= maxCy; ++ny)
        {
            for (int nx = minCx; nx <= maxCx; ++nx)
            {
                const int neighborCell = ny * grid.cellsX + nx;
                const int begin = grid.cellStarts[neighborCell];
                const int end   = grid.cellEnds[neighborCell];

                for (int k = begin; k < end; ++k)
                {
                    const int j = grid.sortedParticleIds[k];
                    if (j == i) continue;

                    const float dx = particles.x[i] - particles.x[j];
                    const float dy = particles.y[i] - particles.y[j];
                    const float dist2 = dx * dx + dy * dy;

                    if (dist2 < h2)
                        neighborIds[cursor[i]++] = j;
                }
            }
        }
    }
}

void SimulationBackendCPU::finalizeVelocities(float dt)
{
    const float invDt = 1.0f / dt;

    for (int i = 0; i < particles.count; ++i)
    {
        particles.vx[i] = (particles.x[i] - particles.px[i]) * invDt;
        particles.vy[i] = (particles.y[i] - particles.py[i]) * invDt;
        particles.vz[i] = (particles.z[i] - particles.pz[i]) * invDt;
    }
}

void SimulationBackendCPU::computeDensity()
{
    const int n = particles.count;
    if (n == 0)
        return;

    const float selfKernel = SPH::poly6(0.0f);

    for (int i = 0; i < n; ++i)
    {
        float rho  = particles.mass[i] * selfKernel;

        const int begin = neighborOffsets[i];
        const int end = neighborOffsets[i + 1];

        const float xi = particles.x[i];
        const float yi = particles.y[i];

        for (int k = begin; k < end; ++k)
        {
            const int j = neighborIds[k];

            const float dx = xi - particles.x[j];
            const float dy = yi - particles.y[j];
            const float r  = std::sqrt(dx * dx + dy * dy);

            rho += particles.mass[j] * SPH::poly6(r);
        }
        particles.density[i] = rho;
    }
}

void SimulationBackendCPU::computeLambda()
{
    const int n = particles.count;
    if (n == 0)
        return;

    const float restDensity = Config::restDensity;
    const float invRestDensity = 1.0f / restDensity;
    const float eps = Config::epsilon;
    const float gradEps = 1e-6f;

    for (int i = 0; i < n; ++i)
    {
        const float rho_i = particles.density[i];
        const float C_i = rho_i * invRestDensity - 1.0f;

        float sumGrad2 = 0.0f;
        float gradCiX = 0.0f;
        float gradCiY = 0.0f;

        const float xi = particles.x[i];
        const float yi = particles.y[i];

        const int begin = neighborOffsets[i];
        const int end   = neighborOffsets[i + 1];

        for (int k = begin; k < end; ++k)
        {
            const int j = neighborIds[k];

            const float dx = xi - particles.x[j];
            const float dy = yi - particles.y[j];
            const float r2 = dx * dx + dy * dy;

            if (r2 <= gradEps)
                continue;

            const float r = std::sqrt(r2);
            const float invR = 1.0f / r;

            const float gradW = SPH::spikyGradCoeff(r);
            const float coeff = particles.mass[j] * invRestDensity * gradW;

            const float gx = coeff * dx * invR;
            const float gy = coeff * dy * invR;

            sumGrad2 += gx * gx + gy * gy;

            gradCiX -= gx;
            gradCiY -= gy;
        }

        sumGrad2 += gradCiX * gradCiX + gradCiY * gradCiY;

        particles.lambda[i] = -C_i / (sumGrad2 + eps);
    }
}

void SimulationBackendCPU::computeDeltaPositions()
{
    const int n = particles.count;
    if (n == 0)
        return;

    const float invRestDensity = 1.0f / Config::restDensity;
    const float gradEps = 1e-6f;

    for (int i = 0; i < n; ++i)
    {
        float deltaX = 0.0f;
        float deltaY = 0.0f;

        const float xi = particles.x[i];
        const float yi = particles.y[i];
        const float lambda_i = particles.lambda[i];

        const int begin = neighborOffsets[i];
        const int end   = neighborOffsets[i + 1];

        for (int k = begin; k < end; ++k)
        {
            const int j = neighborIds[k];

            const float dx = xi - particles.x[j];
            const float dy = yi - particles.y[j];
            const float r2 = dx * dx + dy * dy;

            if (r2 <= gradEps)
               continue;

            const float r = std::sqrt(r2);
            const float invR = 1.0f / r;

            const float gradW = SPH::spikyGradCoeff(r);
            const float coeff = (lambda_i + particles.lambda[j]) * particles.mass[j] * invRestDensity * gradW;

            deltaX += coeff * dx * invR;
            deltaY += coeff * dy * invR;
        }

        particles.dx[i] = deltaX;
        particles.dy[i] = deltaY;
    }
}

void SimulationBackendCPU::applyDeltaPositions()
{
    const int n = particles.count;

    for (int i = 0; i < n; ++i)
    {
        particles.x[i] += 0.001f * particles.dx[i];
        particles.y[i] += 0.001f * particles.dy[i];
    }
    
}


void SimulationBackendCPU::loadScene(const SceneDescription& desc)
{
    Config::gravityX = desc.gravityX;
    Config::gravityY = desc.gravityY;
    Config::gravityZ = desc.gravityZ;
    particles = SceneFiller::fill(desc);
}
