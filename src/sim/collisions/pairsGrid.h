#pragma once
#include "data/particleData.h"
#include "sim/structs.h"
#include "vector"


struct UniformGrid2D 
{
    float left, right, bottom, top;
    float cellSize;
    int cellsX, cellsY;

    // flat storage
    std::vector<std::vector<std::vector<int>>> grid;

    void rebuild(float l, float r, float b, float t, float cs);
    void build(const Particles2D& p);
    void findPairs(const Particles2D& p, float radius, std::vector<CollisionPair>& out) const;
};