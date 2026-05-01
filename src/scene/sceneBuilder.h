#pragma once
#include "scene/sceneDescription.h"

class SceneBuilder {
public:
    // Фабрика 
    static SceneBuilder create(const std::string& name);

    // ───────────────────── Сосуд ─────────────────────

    /// Прямоугольный параллелепипед с центром в pivot
    SceneBuilder& setBoxVessel(float halfX, float halfY, float halfZ);

    /// Выпуклая призма (XZ-полигон, вытянутая по Y)
    SceneBuilder& setConvexPrismVessel(const std::vector<glm::vec2>& polygon, float yMin, float yMax);

    /// Добавить патч к телу сосуда вручную
    SceneBuilder& addVesselPatch(const BoundaryPatch& patch);

    SceneBuilder& addInternalRectBaffle(const glm::vec3& point, const glm::vec3& normal, const glm::vec3& upHint,
        float halfWidth, float halfHeight, float thickness);

    SceneBuilder& addInternalBaffleWithCircularHole( const glm::vec3& point, const glm::vec3& normal, const glm::vec3& upHint, 
        float halfWidth, float halfHeight, float thickness, const glm::vec2& holeCenter, float holeRadius);

    // ─────────────────────  Регионы частиц ─────────────────────
    SceneBuilder& addFluidBox(float cx, float cy, float cz,
                         float halfX, float halfY, float halfZ = 0.f);

    SceneBuilder& addFluidSphere(float cx, float cy, float cz, float radius);

    // Модификаторы последнего региона жидкости
    SceneBuilder& withVelocity(float vx, float vy = 0.f, float vz = 0.f);
    SceneBuilder& withSpacing(float spacing);
    SceneBuilder& withPhase(int phase);
    SceneBuilder& withFilterByBoundary(bool enabled);

    SceneDescription build();

private:
    SceneDescription mdesc;

    /// Возвращает указатель на последний регион; assert если регионов нет
    FluidRegion* last();
};