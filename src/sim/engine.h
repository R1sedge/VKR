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

    const std::vector<int>& getNeighborOffsets() const { return neighborOffsets; }
    const std::vector<int>& getNeighborIds() const { return neighborIds; }

    void reset();

private:
    int iterations = Config::iterations;    
    float velocityDamping = 0.005f;

    Particles2D particles;

    BoxBoundsConstraint2D boxConstraint;
    CircleCollisionConstraint2D circleCollision;

    std::vector<CollisionPair> collisionPairs;
    UniformGrid2D grid;

    // CSR Список соседей для PBF:
    // Соседи частицы i в промежутке [neighborOffsets[i], neighborOffsets[i + 1])
    std::vector<int> neighborOffsets;
    std::vector<int> neighborIds;

private:
    // общие PBD/PBF стадии
    void beginStep();
    void predictPositions(float dt);
        
    void buildBroadphase();
    void buildCollisionPairs();
    void buildNeighbors();

    void finalizeVelocities(float dt);

    // PBF стадии
    void computeDensity();
    void computeLambda();
    void computeDeltaPositions();
    void applyDeltaPositions();
};