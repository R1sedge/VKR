#pragma once
#include "data/particleData.h"
#include "sim/constraints.h"
#include <vector>

class Simulation2D
{
public:
    Simulation2D();

    void update(float dt);  

    void setWorldBounds(float left, float right, float bottom, float top);

    const Particles2D& getParticles() const {return particles;}

private:
    Particles2D particles;

    BoxBoundsConstraint2D boxConstraint;

    std::vector<IConstraint2D*> constraints;

    void integrate(float dt);
    void solveConstraints();

};