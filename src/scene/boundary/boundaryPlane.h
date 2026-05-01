#pragma once
#include <glm/glm.hpp>

// Полуплоскость — базовый строительный блок сосуда.
// Соглашение: signedDist(p) > 0  <=>  точка p находится ВНУТРИ сосуда.
// Нормаль должна быть нормированной и направленной ВНУТРЬ.
struct BoundaryPlane 
{
    glm::vec3 point  = {0.f, 0.f, 0.f};  // любая точка на плоскости
    glm::vec3 normal = {0.f, 1.f, 0.f};  // единичная нормаль, направлена внутрь

    float signedDist(glm::vec3 p) const
    {
        return glm::dot(normal, p - point);
    }
};

// BoundaryPlane + прямоугольные размеры.
// halfWidth  — полуразмер вдоль U-касательной
// halfHeight — полуразмер вдоль V-касательной
struct BoundaryPatch : BoundaryPlane 
{
    float halfWidth = 1.f;
    float halfHeight = 1.f;
};

enum class InternalApertureType
{
    None = 0,
    Circle = 1
};

struct InternalBoundaryPatch
{
    glm::vec3 point;   // Центр перегородки (body-space)
    glm::vec3 normal;  // Нормаль к плоскости (body-space)
    glm::vec3 u;       // Локальная ось X в плоскости патча
    glm::vec3 v;       // Локальная ось Y в плоскости патча

    float halfWidth;
    float halfHeight;
    float thickness;   // Физическая толщина стенки

    InternalApertureType apertureType = InternalApertureType::None;
    glm::vec2 apertureCenter = { 0.f, 0.f }; // В локальных координатах u/v
    float apertureRadius = 0.f;
};