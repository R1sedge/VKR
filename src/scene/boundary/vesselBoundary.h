#pragma once

#include <glm/gtc/quaternion.hpp>
#include <vector>

#include "BoundaryPlane.h"

#include "sim/structs.h"
#include "scene/boundary/vesselWireframe.h"

// Выпуклый сосуд — набор патчей в системе отсчёта тела (body-frame).
// Вращение мышкой → обновить `orientation` → вызвать getWorldPlanes()
// → загрузить результат на GPU через setVesselPlanes().
struct VesselBoundary 
{

    std::vector<BoundaryPatch> bodyPatches;  // патчи в body-frame, не меняются после создания
    std::vector<InternalBoundaryPatch> internalPatches;
    
    VesselWireframe wireframe;

    glm::quat orientation = glm::quat(1.f, 0.f, 0.f, 0.f); // единичный кватернион (нет поворота)
    glm::vec3 pivot = {0.f, 0.f, 0.f}; // точка вращения

    // Трансформировать все body-frame патчи в мировые плоскости.
   std::vector<BoundaryPlane> getWorldPlanes() const;
   
   // Применяет orientation и pivot к point/normal/u/v каждого патча
    std::vector<InternalBoundaryPatch> getWorldInternalPatches() const;

    // Возврашает true, если точка p находится внутри всех полуплоскостей.
    // Переводит p в body-space — не аллоцирует мировые плоскости.
    // margin: отступ от стенки внутрь.
    bool contains(glm::vec3 p, float margin) const;

    // Максимальное расстояние от pivot до любого угла любого патча.
    // Считается в body-frame и не зависит от orientation.
    float computeBoundingRadius() const;

    // Кубический AABB для neighbour-grid вокруг pivot.
    // extraMargin добавляется к радиусу наружу.
    AABB computeGridAABB(float extraMargin) const;

    // ──────────────────── Фабрики ───────────────────────

    // Прямоугольный ящик. Создаёт ровно 6 патчей с нормалями ±X, ±Y, ±Z.
    static VesselBoundary makeBox(glm::vec3 halfExtents);

    // Выпуклая призма: замкнутый 2D-полигон в плоскости XZ
    // (vec2.x = мировой X, vec2.y = мировой Z), вершины перечислены
    // против часовой стрелки при взгляде с +Y, вытянут вдоль оси Y.
    // Создаёт polygon.size() боковых патчей + 2 торцевых крышки.
    static VesselBoundary makeConvexPrism(const std::vector<glm::vec2>& polygon, float yMin, float yMax);
};