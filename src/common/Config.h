#pragma once

namespace Config
{
    // RENDERING / CAMERA
    inline int windowWidth = 1280;
    inline int windowHeight = 720;
    inline constexpr float pixelsPerUnits = 100.0f; // 1 единица физ. коорд. = 100px

    // SIMULATION DEFAULTS
    inline constexpr float dt = 1.0f / 100.0f;     // Шаг симуляции
    inline float gravityX = 0.0f;                // Гравитация по x
    inline float gravityY = -1.0f;                // Гравитация по y
    inline constexpr float particleRadius = 0.05f; // В world units 

    // PBF константы
    inline constexpr float restDensity = 15.0f;  // Плотность воды
    inline constexpr float smoothingRadius = particleRadius * 6.0f; // Радиус ядра сглаживания
    inline constexpr float epsilon = 1e-6f;        // Регуляризация для lambda

    // Настройки PBD решателя
    inline constexpr int iterations = 4;
}