#pragma once

#include <cmath>

#include "common/Config.h"

namespace CpuSPH
{
    inline constexpr float kPi = 3.14159265358979323846f;

    // 3D Poly6 kernel.
    inline float poly6(float r, float h = Config::smoothingRadius)
    {
        if (r < 0.0f || r >= h)
            return 0.0f;

        const float h2 = h * h;
        const float h4 = h2 * h2;
        const float h8 = h4 * h4;
        const float h9 = h8 * h;

        const float k = 315.0f / (64.0f * kPi * h9);

        const float x = h2 - r * r;
        return k * x * x * x;
    }

    // Возвращает скалярный коэффициент 3D Spiky gradient:
    // grad W = spikyGradCoeff(r, h) * (rij / r)
    inline float spikyGradCoeff(float r, float h = Config::smoothingRadius)
    {
        if (r < 0.0f || r >= h)
            return 0.0f;

        const float h6 = h * h * h * h * h * h;
        const float k = -45.0f / (kPi * h6);

        const float x = h - r;
        return k * x * x;
    }
}