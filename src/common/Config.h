#pragma once

namespace Config
{
    // RENDERING / CAMERA
    inline int windowWidth = 1280;
    inline int windowHeight = 720;
    inline constexpr float pixelsPerUnits = 100.0f; // 1 единица физ. коорд. = 100px

    // SIMULATION DEFAULTS
    inline constexpr float dt = 1.0f / 120.0f;    // Шаг симуляции
    inline float gravityY = -9.81f;               // Гравитация по y
    inline constexpr float particleRadius = 0.05f; // В world units (при ppu=100 это 5px)

    // PBD SOLVING PARAMS
    inline constexpr int iterations = 4;
}