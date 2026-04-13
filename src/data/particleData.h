#pragma once
#include <vector>

struct Particles2D
{
    int count = 0;
    
    // Позиции
    std::vector<float> x, y, z;   // Предсказанные / Текущие позиции
    std::vector<float> px, py, pz; // Позиции на начало шага

    // Скорости
    std::vector<float> vx, vy, vz;

    // Масса
    std::vector<float> mass;

    // PBF буфферы
    std::vector<float> density; // Плотности
    std::vector<float> lambda;  // Множители Лагранжа
    std::vector<float> dx, dy, dz;  // коррекции позиции

    std::vector<int> phase; // id типы жидкости

    void resize(int n);
    void clearDerived();
};