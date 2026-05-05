#pragma once
#include <glm/glm.hpp>

namespace Config
{
    // RENDERING / CAMERA
    inline int windowWidth = 1600;
    inline int windowHeight = 900;
    inline float worldDepth = 3.0f;    // Глубина мира (world units)
    inline float fovY = 60.0f;         // Угол обзора камеры (градусы)
    inline float cameraDist = 5.0f;    // Начальное расстояние камеры до цели

    inline glm::vec4 phase0Color = glm::vec4(0.1f, 0.6f, 0.9f, 1.0f);
    inline glm::vec4 phase1Color = glm::vec4(0.92f, 0.88f, 0.7f, 1.0f);

    inline float maxGradSpeed = 4.0f;

    inline int particleColorMode = 0; // 0 = speed colormap, 1 = phase colors

    // Константы симуляции
    inline float dt = 1.0f / 90.0f;     // Шаг симуляции
    inline float gravityX = 0.0f;       // Гравитация по x
    inline float gravityY = -9.81f;     // Гравитация по y
    inline float gravityZ = 0.0f;       // Гравитация по z

    inline float wallRestitution = 0.80f;
    inline float wallFriction = 0.05f;

    inline constexpr float particleRadius = 0.025f; // В world units 
    inline float particleMass = 0.25f;              // В kg

    // PBF константы
    inline float restDensity = 1000.0f;                             // Плотность воды (3D)
    inline constexpr float smoothingRadius = particleRadius * 5.0f; // Радиус ядра сглаживания
    inline constexpr float epsilon = 500.f;                         // Регуляризация для lambda

    // Artificial Pressure
    inline float artificialPressureK = 0.001f;
    inline float artificialPressureDeltaQ = 0.06f;

    // Vorticity Confinement
    inline float vorticityEpsilon = 0.6f;

    // Xsph
    inline float xsphViscosity = 0.01f;

    // Настройки PBD решателя
    inline int iterations = 2;

    // Ограничение скорости
    inline float maxSpeed = 8.0f;

     // Внутренние перегородки
    inline bool enableBafflePairFiltering = true;
}