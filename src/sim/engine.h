#pragma once
#include <vector>

#include "data/particleData.h"
#include "sim/constraints/boxBounds.h"
#include "sim/constraints/circleCollision.h"
#include "sim/structs.h"
#include "collisions/pairsGrid.h"

class Simulation2D
{
public:
    Simulation2D();

    void update(float dt);  

    void setWorldBounds(float left, float right, float bottom, float top);
    void setIterations(int iter) { iterations = iter; }
    void configureGrid(float left, float right, float bottom, float top, float cellSize);
    void setVelocityDamping(float d) { velocityDamping = d; }

    const Particles2D& getParticles() const {return particles;}

    void reset();

    bool useGrid = true; // Потом прокинем в чекбокс в ui

private:
    int iterations = 4;    
    float velocityDamping = 0.0f;

    Particles2D particles;

    BoxBoundsConstraint2D boxConstraint;
    CircleCollisionConstraint2D circleCollision;

    std::vector<CollisionPair> collisionPairs;
    UniformGrid2D grid;

private:
    // общие PBD/PBF стадии
    void beginStep();
    void predictPositions(float dt);
    void buildCollisionPairs();
    void buildNeighbors();
    void solveSolidConstraints();
    void finalizeVelocities(float dt);

    // PBF стадии
    void computeDensity();
    void computeLambda();
    void computeDeltaPositions();
    void applyDeltaPositions();
};