#pragma once
#include "data/particleData.h"
#include "sim/structs.h"
#include "common/config.h"

class CircleCollisionConstraint2D
{
public:
    CircleCollisionConstraint2D(float radi = Config::particleRadius): radius(radi){};

    void project(Particles3D& particles, std::vector<CollisionPair>& out);

private:
    float radius;
};
