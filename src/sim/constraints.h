#pragma once
#include "data/particleData.h"
#include "common/config.h"

class IConstraint2D // Виртуальный класс для 2D ограничений
{
public:
    virtual ~IConstraint2D() = default;

    // Проецирование ограничений
    virtual void project(Particles2D& particles) = 0;
};

class BoxBoundsConstraint2D : public IConstraint2D
{
public:
    BoxBoundsConstraint2D(float l, float r, float b, float t,
                          float radi = Config::particleRadius):
        left(l), right(r), bottom(b), top(t), radius(radi)
        {}
    
    void setBounds(float l, float r, float b, float t)
    {
        left = l;
        right = r;
        bottom = b;
        top = t;
    }

    void project(Particles2D& particles)
    {
        for (int i = 0; i < particles.count; ++i)
        {
            float& x = particles.x[i];
            float& y = particles.y[i];

            if (x < left + radius) x = left + radius;
            if (x > right - radius) x = right - radius;
            if (y < bottom + radius) y = bottom + radius;
            if (y > top - radius) y = top - radius;
        }
    }

private:
    float left, right, bottom, top;
    float radius;
};