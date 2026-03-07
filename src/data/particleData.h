#pragma once
#include <vector>

struct Particles2D
{
    int count = 0;
    
    // Позиции
    std::vector<float> x, y;   // Предсказанные / Текущие позиции
    std::vector<float> px, py; // Позиции на начало шага

    // Скорости
    std::vector<float> vx, vy;

    // Масса
    std::vector<float> mass;

    // PBF буфферы
    std::vector<float> density; // Плотности
    std::vector<float> lambda;  // Множители Лагранжа
    std::vector<float> dx, dy;  // коррекции позиции

    void resize(int n);
    void clearDerived();
};