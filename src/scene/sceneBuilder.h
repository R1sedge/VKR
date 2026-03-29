#pragma once
#include "scene/sceneDescription.h"

class SceneBuilder 
{
public:
    static SceneBuilder create(const std::string& name = ""); // Статический фабричный метод

    // Добавление регионов
    SceneBuilder& addRect(float cx, float cy, float halfW, float halfH);
    SceneBuilder& addCircle(float cx, float cy, float radius);

    // Модификаторы последнего добавленного региона
    SceneBuilder& withVelocity(float vx, float vy);
    SceneBuilder& withMass(float mass);
    SceneBuilder& withSpacing(float spacing);
    SceneBuilder& withPhase(int phase);

    // Глобальные параметры
    SceneBuilder& setGravity(float gx, float gy);

    SceneDescription build();

    private:
    SceneDescription m_desc;
    // для модификаторов последнего региона
    ParticleRegion* last();
};