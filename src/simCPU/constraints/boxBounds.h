#pragma once
#include "data/particleData.h"
#include "common/config.h"

class BoxBoundsConstraint2D
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

    void project(Particles3D& particles);

private:
    float left, right, bottom, top;
    float radius;
};