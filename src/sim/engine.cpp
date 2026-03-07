#include "engine.h"
#include "common/config.h"
#include "collisions/pairsNaive.h"
#include "collisions/pairsGrid.h"

Simulation2D::Simulation2D() : boxConstraint(0.0f, 0.0f, 0.0f, 0.0f),
    circleCollision(Config::particleRadius)
{
    reset();
}

void Simulation2D::reset()
{
    const float r = Config::particleRadius;
    const float step = r * 2.1f; // небольшой зазор между частицами

    const int cols = 125;
    const int rows = 16;
    const int n = cols * rows;

    particles.resize(n);
    collisionPairs.reserve(n * 8); // Верхняя оценка числа пар
    collisionPairs.clear();

   for (int row = 0; row < rows; ++row)
    {
        for (int col = 0; col < cols; ++col)
        {
            int idx = row * cols + col;

            float fx = (col - cols / 2) * step;
            float fy = (row - rows / 2) * step + 1.5f; // чуть выше центра

            particles.x[idx]    = fx;
            particles.y[idx]    = fy;
            particles.px[idx]   = fx; // Verlet: p_prev = p_current
            particles.py[idx]   = fy;

            particles.vx[idx]   = 0.0f;
            particles.vy[idx]   = 0.0f;
            
            particles.mass[idx] = 1.0f;
        }
    }
    particles.clearDerived();
}

void Simulation2D::setWorldBounds(float left, float right, float bottom, float top)
{
    boxConstraint.setBounds(left, right, bottom, top);

    const float cellSize = 2.0f * Config::particleRadius;
    configureGrid(left, right, bottom, top, cellSize);
}

void Simulation2D::configureGrid(float left, float right, float bottom, float top, float cellSize)
{
    grid.rebuild(left, right, bottom, top, cellSize);
}

void Simulation2D::update(float dt)
{
    if (particles.count == 0)
        return;

    beginStep();
    predictPositions(dt);
    
    buildNeighbors();

    for (int iter = 0; iter < iterations; ++iter)
    {
        solveSolidConstraints();

        computeDensity();
        computeLambda();
        computeDeltaPositions();
        applyDeltaPositions();

        boxConstraint.project(particles); // Чтобы все частицы оставались внутри границ
    }
    finalizeVelocities(dt); // Обновление скоростей
}

void Simulation2D::beginStep()
{
    particles.clearDerived();
}

void Simulation2D::predictPositions(float dt)
{
    const float gx = 0.0f;
    const float gy = Config::gravityY;

    for (int i = 0; i < particles.count; ++i)
    {
        float x  = particles.x[i];
        float y  = particles.y[i];
        float px = particles.px[i];
        float py = particles.py[i];

        float vx = x - px;
        float vy = y - py;

        vx += gx * dt * dt;
        vy += gy * dt * dt;

        particles.px[i] = x;
        particles.py[i] = y;

        particles.x[i]  = x + vx;
        particles.y[i]  = y + vy;
    }
}

void Simulation2D::buildCollisionPairs()
{
    collisionPairs.clear();

    if (useGrid)
    {
        grid.build(particles); 
        grid.findPairs(particles, Config::particleRadius, collisionPairs); 
    }
    else
    {
        findPairsNaive(particles, Config::particleRadius, collisionPairs);
    }
}

void Simulation2D::buildNeighbors()
{
    // Пока заглушка.
    // Для PBF здесь будет отдельный neighbor list по support radius h,
    // причём строиться он должен один раз перед solver-итерациями.
}

void Simulation2D::solveSolidConstraints()
{
    buildCollisionPairs();
    circleCollision.project(particles, collisionPairs);
    boxConstraint.project(particles);
}

void Simulation2D::finalizeVelocities(float dt)
{
    const float invDt = 1.0f / dt;
    const float damp = velocityDamping;

    for (int i = 0; i < particles.count; ++i)
    {
        float vx = (particles.x[i] - particles.px[i]) * invDt;
        float vy = (particles.y[i] - particles.py[i]) * invDt;

        // Опционально damping
        vx *= (1.0f - damp);
        vy *= (1.0f - damp);

        particles.vx[i] = vx;
        particles.vy[i] = vy;
    }
}

void Simulation2D::computeDensity()
{

}

void Simulation2D::computeLambda()
{
    
}

void Simulation2D::computeDeltaPositions()
{
    
}

void Simulation2D::applyDeltaPositions()
{
    
}
