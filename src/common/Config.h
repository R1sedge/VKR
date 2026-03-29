#pragma once

namespace Config
{
    // RENDERING / CAMERA
    inline int windowWidth = 1280;
    inline int windowHeight = 720;
    inline constexpr float pixelsPerUnits = 100.0f; // 1 единица физ. коорд. = 100px

    // Константы симуляции
    inline constexpr float dt = 1.0f / 100.0f;     // Шаг симуляции
    inline float gravityX = 0.0f;                  // Гравитация по x
    inline float gravityY = -6.0f;                 // Гравитация по y
    inline constexpr float particleRadius = 0.04f; // В world units 

    // PBF константы
    inline float restDensity = 200.0f;                              // Плотность воды
    inline constexpr float smoothingRadius = particleRadius * 5.0f; // Радиус ядра сглаживания
    inline constexpr float epsilon = 150.f;                         // Регуляризация для lambda

    // Artificial Pressure
    inline constexpr float artificialPressureK = 0.008f;
    inline constexpr float artificialPressureDeltaQ = 0.05f;

    // Vorticity Confinement
    inline float vorticityEpsilon = 0.10f;

    // Xsph
    inline float xsphViscosity = 0.01f;

    // Настройки PBD решателя
    inline constexpr int iterations = 6;
}