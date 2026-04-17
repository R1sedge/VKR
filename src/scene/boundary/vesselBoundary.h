#pragma once
#include "BoundaryPlane.h"
#include <glm/gtc/quaternion.hpp>
#include <vector>

// Выпуклый сосуд — набор патчей в системе отсчёта тела (body-frame).
// Вращение мышкой → обновить `orientation` → вызвать getWorldPlanes()
// → загрузить результат на GPU через setVesselPlanes().
struct VesselBoundary 
{

    std::vector<BoundaryPatch> bodyPatches;  // патчи в body-frame, не меняются после создания

    glm::quat orientation = glm::quat(1.f, 0.f, 0.f, 0.f); // единичный кватернион (нет поворота)
    glm::vec3 pivot = {0.f, 0.f, 0.f}; // точка вращения

    // Трансформировать все body-frame патчи в мировые плоскости.
    [[nodiscard]] std::vector<BoundaryPlane> getWorldPlanes() const;

    // Возврашает true, если точка p находится внутри всех полуплоскостей.
    // Переводит p в body-space — не аллоцирует мировые плоскости.
    // margin: отступ от стенки внутрь.
    [[nodiscard]] bool contains(glm::vec3 p, float margin = 0.f) const noexcept;

    // ──────────────────── Фабрики ───────────────────────

    // Прямоугольный ящик. Создаёт ровно 6 патчей с нормалями ±X, ±Y, ±Z.
    [[nodiscard]] static VesselBoundary makeBox(glm::vec3 halfExtents,
                                                glm::vec3 pivot = {0.f, 0.f, 0.f});

    // Выпуклая призма: замкнутый 2D-полигон в плоскости XZ
    // (vec2.x = мировой X, vec2.y = мировой Z), вершины перечислены
    // против часовой стрелки при взгляде с +Y, вытянут вдоль оси Y.
    // Создаёт polygon.size() боковых патчей + 2 торцевых крышки.
    [[nodiscard]] static VesselBoundary makeConvexPrism(const std::vector<glm::vec2>& polygon,
                                                        float yMin, float yMax,
                                                        glm::vec3 pivot = {0.f, 0.f, 0.f});
};