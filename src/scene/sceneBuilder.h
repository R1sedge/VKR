#pragma once
#include "scene/sceneDescription.h"

class SceneBuilder 
{
public:
    static SceneBuilder create(const std::string& name = ""); // Статический фабричный метод

    // Добавление регионов
    SceneBuilder& addRect(float cx, float cy, float cz, float halfW, float halfH, float halfD = 0.0f);
    SceneBuilder& addCircle(float cx, float cy, float cz, float radius);

    // Модификаторы последнего добавленного региона
    SceneBuilder& withVelocity(float vx, float vy, float vz = 0.0f);
    SceneBuilder& withMass(float mass);
    SceneBuilder& withSpacing(float spacing);
    SceneBuilder& withPhase(int phase);

    // Глобальные параметры
    SceneBuilder& setGravity(float gx, float gy, float gz = 0.0f);

    SceneDescription build();

    private:
    SceneDescription m_desc;
    // для модификаторов последнего региона
    ParticleRegion* last();
};