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
    void setSubSteps(int iter) { subSteps = iter; }

    void setVelocityDamping(float d) { velocityDamping = d; }

    const Particles2D& getParticles() const {return particles;}

private:
    int subSteps = 4;    
     float velocityDamping = 0.0f;

    Particles2D particles;
    BoxBoundsConstraint2D boxConstraint;
    std::vector<IConstraint2D*> constraints;

    void integrate(float dt);
    void solveConstraints();
    void finalize(float dt);

};