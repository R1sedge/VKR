#pragma once

namespace Config
{
    // RENDERING / CAMERA
    inline int windowWidth = 1280;
    inline int windowHeight = 720;
    inline constexpr float pixelsPerUnits = 100.0f; // 1 единица физ. коорд. = 100px

    // Константы симуляции
    inline constexpr float dt = 1.0f / 100.0f;      // Шаг симуляции
    inline float gravityX = 0.0f;                  // Гравитация по x
    inline float gravityY = -5.0f;                 // Гравитация по y
    inline constexpr float particleRadius = 0.04f; // В world units 

    // PBF константы
    inline float restDensity = 150.0f;  // Плотность воды
    inline constexpr float smoothingRadius = particleRadius * 5.0f; // Радиус ядра сглаживания
    inline constexpr float epsilon = 1e-6f;        // Регуляризация для lambda

    inline constexpr float artificialPressureK = 0.2f;
    inline constexpr float artificialPressureDeltaQ = 0.1f;

    // Настройки PBD решателя
    inline constexpr int iterations = 6;
}