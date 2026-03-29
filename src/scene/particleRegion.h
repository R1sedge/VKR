#pragma once
#include <cmath>

enum class RegionShape
{
    Rect,   // прямоугольник, задаётся центром + полуразмерами
    Circle  // круг, задаётся центром + радиусом
};

struct ParticleRegion
{
    RegionShape shape = RegionShape::Rect;

    float cx = 0.f; // центр по X (world units)
    float cy = 0.f; // центр по Y (world units)

    float halfW = 1.f;
    float halfH = 1.f;

    float radius = 1.f;

    float vx = 0.f; // начальная скорость по X
    float vy = 0.f; // начальная скорость по Y

    float mass = 1.f; // масса каждой частицы в регионе

    
    float spacing = 0.f; // Шаг сетки заполнения.

    int phase = 0; // id фазы жидкости
};