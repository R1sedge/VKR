#pragma once
#include <glm/glm.hpp>

// Полуплоскость — базовый строительный блок сосуда.
// Соглашение: signedDist(p) > 0  <=>  точка p находится ВНУТРИ сосуда.
// Нормаль должна быть нормированной и направленной ВНУТРЬ.
struct BoundaryPlane 
{
    glm::vec3 point  = {0.f, 0.f, 0.f};  // любая точка на плоскости
    glm::vec3 normal = {0.f, 1.f, 0.f};  // единичная нормаль, направлена внутрь

    [[nodiscard]] float signedDist(glm::vec3 p) const noexcept 
    {
        return glm::dot(normal, p - point);
    }
};

// BoundaryPlane + прямоугольные размеры.
// halfWidth  — полуразмер вдоль U-касательной
// halfHeight — полуразмер вдоль V-касательной
struct BoundaryPatch : BoundaryPlane {
    float halfWidth  = 1.f;
    float halfHeight = 1.f;
};