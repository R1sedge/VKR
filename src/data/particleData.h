#pragma once
#include <vector>

struct Particles2D
{
    int count = 0;
    
    std::vector<float> x, y;
    std::vector<float> px, py;
    std::vector<float> vx, vy;
    std::vector<float> mass;

    void resize(int n);
};