// scene/FluidRegion.h
#pragma once
#include <cmath>

enum class RegionShape { Rect, Sphere };

struct FluidRegion 
{
    RegionShape shape = RegionShape::Rect;

    // Позиции / размеры
    float cx = 0.f, cy = 0.f, cz = 0.f;
    float halfX = 1.f, halfY = 1.f, halfZ = 1.f;
    float radius = 1.f;

    // Initial conditions
    float vx = 0.f, vy = 0.f, vz = 0.f;
    float spacing = 0.f; // 0 → автоматически из Config::particleRadius
    int phase = 0;

    // Фильтрация по VesselBoundary при заполнении
    bool filterByBoundary = true;
};