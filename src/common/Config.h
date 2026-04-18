#pragma once

namespace Config
{
    // RENDERING / CAMERA
    inline int windowWidth = 1280;
    inline int windowHeight = 720;
    inline float worldDepth = 3.0f;                 // Глубина мира (world units)
    inline float fovY = 60.0f;                      // Угол обзора камеры (градусы)
    inline float cameraDist = 5.0f;                 // Начальное расстояние камеры до цели

    // Константы симуляции
    inline constexpr float dt = 1.0f / 80.0f;     // Шаг симуляции
    inline float gravityX = 0.0f;                  // Гравитация по x
    inline float gravityY = -8.0f;                 // Гравитация по y
    inline float gravityZ = 0.0f;                  // Гравитация по z

    inline constexpr float particleRadius = 0.1f; // В world units 
    inline float particleMass = 2.00f;              // В kg

    // PBF константы
    inline float restDensity = 100.0f;                             // Плотность воды (3D)
    inline constexpr float smoothingRadius = particleRadius * 5.0f; // Радиус ядра сглаживания
    inline constexpr float epsilon = 50.f;                          // Регуляризация для lambda

    // Artificial Pressure
    inline constexpr float artificialPressureK = 0.005f;
    inline constexpr float artificialPressureDeltaQ = 0.1f;

    // Vorticity Confinement
    inline float vorticityEpsilon = 0.10f;

    // Xsph
    inline float xsphViscosity = 0.05f;

    // Настройки PBD решателя
    inline constexpr int iterations = 6;

    // Ограничение скорости
    inline float maxSpeed = 32.0f;
}