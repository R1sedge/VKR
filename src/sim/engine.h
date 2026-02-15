#pragma once
#include "data/particleData.h"

class Simulation2D
{
public:
    Simulation2D();

    void step(float dt);

    void setWorldBounds(float left, float right, float bottom, float top);

private:
    Particles2D particles;

    float left, right, bottom, top;

    void integrate(float dt);
    void solveConstraints();

};