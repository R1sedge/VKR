#include "particleData.h"

void Particles2D::resize(int n)
{
    x.resize(n);
    y.resize(n);
    px.resize(n);
    py.resize(n);
    vx.resize(n);
    vy.resize(n);
    mass.resize(n);
}