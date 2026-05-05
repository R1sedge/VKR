#pragma once

#include <vector>

struct Particles3D
{
    int count = 0;

    // Позиции
    std::vector<float> x, y, z;     // Предсказанные / текущие позиции
    std::vector<float> px, py, pz;  // Позиции на начало шага

    // Скорости
    std::vector<float> vx, vy, vz;

    // Масса
    std::vector<float> mass;

    // PBF буферы
    std::vector<float> density;
    std::vector<float> lambda;
    std::vector<float> dx, dy, dz;

    // Vorticity confinement
    std::vector<float> omegaX;
    std::vector<float> omegaY;
    std::vector<float> omegaZ;

    // id типа жидкости
    std::vector<int> phase;

    void resize(int n);
    void clearDerived();
};