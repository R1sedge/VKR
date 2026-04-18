#include "VesselBoundary.h"
#include <glm/gtc/quaternion.hpp>
#include <cassert>
#include <cmath>
#include <algorithm>

namespace
{
    glm::vec3 makePatchTangent(const glm::vec3& normal)
    {
        const glm::vec3 n = glm::normalize(normal);

        // Выбираем опорную ось, не почти параллельную нормали.
        const glm::vec3 ref =
            (std::abs(n.y) < 0.999f)
                ? glm::vec3(0.f, 1.f, 0.f)
                : glm::vec3(1.f, 0.f, 0.f);

        return glm::normalize(glm::cross(ref, n));
    }

    void buildPatchBasis(const glm::vec3& normal, glm::vec3& u, glm::vec3& v)
    {
        const glm::vec3 n = glm::normalize(normal);
        u = makePatchTangent(n);
        v = glm::normalize(glm::cross(n, u));
    }
}

std::vector<BoundaryPlane> VesselBoundary::getWorldPlanes() const 
{
    std::vector<BoundaryPlane> result;
    result.reserve(bodyPatches.size());

    for (const auto& patch : bodyPatches) 
    {
        BoundaryPlane wp;
        // Поворачиваем нормаль из body-frame в world-frame
        wp.normal = orientation * patch.normal;
        // Поворачиваем точку на плоскости относительно pivot
        wp.point = pivot + orientation * (patch.point - pivot);
        result.push_back(wp);
    }

    return result;
}

bool VesselBoundary::contains(glm::vec3 p, float margin) const
{
    // Переводим p в body-space обратным поворотом
    const glm::vec3 localP = glm::conjugate(orientation) * (p - pivot) + pivot;

    for (const auto& patch : bodyPatches) 
    {
        // Если расстояние до плоскости меньше margin — частица снаружи
        if (patch.signedDist(localP) < margin)
            return false;
    }
    return true;
}

float VesselBoundary::computeBoundingRadius() const
{
    float maxRadius = 0.f;

    for (const BoundaryPatch& patch : bodyPatches)
    {
        glm::vec3 u, v;
        buildPatchBasis(patch.normal, u, v);

        const glm::vec3 c0 = patch.point + u * patch.halfWidth + v * patch.halfHeight;
        const glm::vec3 c1 = patch.point + u * patch.halfWidth - v * patch.halfHeight;
        const glm::vec3 c2 = patch.point - u * patch.halfWidth + v * patch.halfHeight;
        const glm::vec3 c3 = patch.point - u * patch.halfWidth - v * patch.halfHeight;

        maxRadius = std::max(maxRadius, glm::length(c0 - pivot));
        maxRadius = std::max(maxRadius, glm::length(c1 - pivot));
        maxRadius = std::max(maxRadius, glm::length(c2 - pivot));
        maxRadius = std::max(maxRadius, glm::length(c3 - pivot));
    }

    return maxRadius;
}

AABB VesselBoundary::computeGridAABB(float extraMargin) const
{
    const float r = computeBoundingRadius() + std::max(0.f, extraMargin);

    return AABB{
        pivot.x - r, pivot.x + r,
        pivot.y - r, pivot.y + r,
        pivot.z - r, pivot.z + r
    };
}

VesselBoundary VesselBoundary::makeBox(glm::vec3 h) 
{
    VesselBoundary v;

    // Вспомогательная лямбда функция: добавить один патч
    auto add = [&](glm::vec3 point, glm::vec3 inward, float hw, float hh) 
    {
        BoundaryPatch p;
        p.point = point;
        p.normal = inward; // для осевых граней уже нормированы
        p.halfWidth = hw;
        p.halfHeight = hh;
        v.bodyPatches.push_back(p);
    };

    //    точка на грани      нормаль внутрь    hw    hh
    add({ h.x, 0.f, 0.f},  {-1.f,  0.f,  0.f},  h.z,  h.y);  // грань +X
    add({-h.x, 0.f, 0.f},  { 1.f,  0.f,  0.f},  h.z,  h.y);  // грань -X
    add({ 0.f, h.y, 0.f},  { 0.f, -1.f,  0.f},  h.x,  h.z);  // крышка +Y
    add({ 0.f,-h.y, 0.f},  { 0.f,  1.f,  0.f},  h.x,  h.z);  // дно -Y
    add({ 0.f, 0.f, h.z},  { 0.f,  0.f, -1.f},  h.x,  h.y);  // грань +Z
    add({ 0.f, 0.f,-h.z},  { 0.f,  0.f,  1.f},  h.x,  h.y);  // грань -Z

    return v;
}

// ──────────────────────── makeConvexPrism ────────────────────────
// Вывод формулы внутренней нормали для ребра A→B полигона в плоскости XZ:
//   edge  = (B.x - A.x,  B.z - A.z) — направление ребра
//   В 3D: normal3D = normalize(-edgeZ, 0, edgeX)
VesselBoundary VesselBoundary::makeConvexPrism(const std::vector<glm::vec2>& polygon,float yMin, float yMax)
{
    assert(polygon.size() >= 3 && "Полигон должен содержать не менее 3 вершин");
    assert(yMax > yMin && "yMax должен быть больше yMin");

    VesselBoundary v;

    const int n  = static_cast<int>(polygon.size());
    const float halfHeight = (yMax - yMin) * 0.5f;  // полувысота крышек
    const float midY = (yMin + yMax) * 0.5f;  // центр по Y

    // Боковые грани
    for (int i = 0; i < n; ++i) 
    {
        const glm::vec2 a = polygon[i];
        const glm::vec2 b = polygon[(i + 1) % n];
        const glm::vec2 edge = b - a;

        // Внутренняя нормаль: левый перпендикуляр ребра в плоскости XZ
        const glm::vec3 inward = glm::normalize(glm::vec3(-edge.y, 0.f, edge.x));

        BoundaryPatch p;
        p.point = {(a.x + b.x) * 0.5f, midY, (a.y + b.y) * 0.5f}; // центр ребра
        p.normal = inward;
        p.halfWidth = glm::length(edge) * 0.5f;  // половина длины ребра
        p.halfHeight = halfHeight;
        v.bodyPatches.push_back(p);
    }

    // Радиус описанной окружности полигона — для размеров торцевых крышек
    float maxR = 0.f;
    for (const auto& vert : polygon)
        maxR = std::max(maxR, glm::length(vert));

    // Нижняя крышка 
    {
        BoundaryPatch bot;
        bot.point = {0.f, yMin, 0.f};
        bot.normal = {0.f,  1.f, 0.f};
        bot.halfWidth = maxR;
        bot.halfHeight = maxR;
        v.bodyPatches.push_back(bot);
    }

    // Верхняя крышка 
    {
        BoundaryPatch top;
        top.point = {0.f, yMax, 0.f};
        top.normal = {0.f, -1.f, 0.f};
        top.halfWidth = maxR;
        top.halfHeight = maxR;
        v.bodyPatches.push_back(top);
    }

    return v;
}