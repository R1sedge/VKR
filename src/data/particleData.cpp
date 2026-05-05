#include "particleData.h"

#include <algorithm>

void Particles3D::resize(int n)
{
    x.resize(n);
    y.resize(n);
    z.resize(n);

    px.resize(n);
    py.resize(n);
    pz.resize(n);

    vx.resize(n);
    vy.resize(n);
    vz.resize(n);

    mass.resize(n);

    density.resize(n);
    lambda.resize(n);

    dx.resize(n);
    dy.resize(n);
    dz.resize(n);

    omegaX.resize(n);
    omegaY.resize(n);
    omegaZ.resize(n);

    phase.resize(n);

    count = n;
}

void Particles3D::clearDerived()
{
    std::fill(density.begin(), density.end(), 0.0f);
    std::fill(lambda.begin(), lambda.end(), 0.0f);

    std::fill(dx.begin(), dx.end(), 0.0f);
    std::fill(dy.begin(), dy.end(), 0.0f);
    std::fill(dz.begin(), dz.end(), 0.0f);

    std::fill(omegaX.begin(), omegaX.end(), 0.0f);
    std::fill(omegaY.begin(), omegaY.end(), 0.0f);
    std::fill(omegaZ.begin(), omegaZ.end(), 0.0f);
}