#include "engine.h"
#include "common/config.h"

Simulation2D::Simulation2D() : boxConstraint(0.0f, 0.0f, 0.0f, 0.0f)
{
    // Временно
    int n = 100;
    particles.resize(n);
    particles.count = n;

    for (int i = 0; i < n; ++i)
    {
        float fx = -4.0f + 8.0f * (float(i) / float(n - 1)); // [-4, 4]
        float fy = -1.0f + 2.0f * (float(i) / float(n - 1));  // [-1, 1]

        particles.x[i]  = fx;
        particles.y[i]  = fy;
        particles.px[i] = fx; // Verlet: начальная prev = current
        particles.py[i] = fy;

        particles.vx[i] = 0.0f;
        particles.vy[i] = 0.0f;
        particles.mass[i] = 1.0f; // пока одинаковая масса
    }

    constraints.push_back(&boxConstraint);
}

void Simulation2D::setWorldBounds(float left, float right, float bottom, float top)
{
    boxConstraint.setBounds(left, right, bottom, top);
}

void Simulation2D::update(float dt)
{
    if (particles.count == 0)
        return;

    integrate(dt); // Предсказание позиции

    for (int iter = 0; iter < subSteps; ++iter)
        solveConstraints(); 

    finalize(dt);
}

void Simulation2D::integrate(float dt)
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

        float nx = x + vx;
        float ny = y + vy;

        particles.px[i] = x;
        particles.py[i] = y;
        particles.x[i]  = nx;
        particles.y[i]  = ny;
    }
}

void Simulation2D::finalize(float dt)
{
    const float invDt = 1.0f / dt;
    const float damp = velocityDamping;

    for (int i = 0; i < particles.count; ++i)
    {
        float vx = (particles.x[i] - particles.px[i]) * invDt;
        float vy = (particles.y[i] - particles.py[i]) * invDt;

        // Опционально damping
        vx *= (1 - damp);
        vy *= (1 - damp);

        particles.vx[i] = vx;
        particles.vy[i] = vy;
    }
}

void Simulation2D::solveConstraints()
{
    for (IConstraint2D* c : constraints)
        c->project(particles);
}