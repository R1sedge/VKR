#pragma once
#include "scene/sceneDescription.h"

class SceneBuilder {
public:
    // Фабрика 
    static SceneBuilder create(const std::string& name);

    // ───────────────────── Этап 1: Сосуд ─────────────────────

    /// Прямоугольный параллелепипед с центром в pivot
    SceneBuilder& setBoxVessel(float halfX, float halfY, float halfZ);

    /// Выпуклая призма (XZ-полигон, вытянутая по Y)
    SceneBuilder& setConvexPrismVessel(const std::vector<glm::vec2>& polygon, float yMin, float yMax);

    /// Добавить патч к телу сосуда вручную
    SceneBuilder& addVesselPatch(const BoundaryPatch& patch);

    // ───────────────────── Этап 2: Регионы частиц ─────────────────────
    SceneBuilder& addFluidBox(float cx, float cy, float cz,
                         float halfX, float halfY, float halfZ = 0.f);

    SceneBuilder& addFluidSphere(float cx, float cy, float cz, float radius);

    // Модификаторы последнего региона жидкости
    SceneBuilder& withVelocity(float vx, float vy = 0.f, float vz = 0.f);
    SceneBuilder& withSpacing(float spacing);
    SceneBuilder& withPhase(int phase);
    SceneBuilder& withFilterByBoundary(bool enabled);

    // ───────────────────── Финал ─────────────────────
    SceneDescription build();

private:
    SceneDescription mdesc;

    /// Возвращает указатель на последний регион; assert если регионов нет
    FluidRegion* last();
};