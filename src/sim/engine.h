#pragma once
#include "data/particleData.h"
#include "sim/constraints/boxBounds.h"
#include "sim/constraints/circleCollision.h"
#include <vector>
#include "structs.h"

class Simulation2D
{
public:
    Simulation2D();

    void update(float dt);  

    void setWorldBounds(float left, float right, float bottom, float top);
    void setIterations(int iter) { iterations = iter; }

    void setVelocityDamping(float d) { velocityDamping = d; }

    const Particles2D& getParticles() const {return particles;}

private:
    int iterations = 4;    
    float velocityDamping = 0.0f;

    Particles2D particles;
    BoxBoundsConstraint2D boxConstraint;
    CircleCollisionConstraint2D circleCollision;

    std::vector<CollisionPair> collisionPairs;

    void integrate(float dt);
    void solveConstraints();
    void finalize(float dt);

};