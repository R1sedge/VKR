#pragma once

#include "common/config.h"

namespace SPH
{ 
    inline constexpr float kPi = 3.14159265358979323846f;

    // Poly6 ядро для плотности
    inline float poly6(float r, float h = Config::smoothingRadius)
    {
        if (r < 0.0f || r >= h)
            return 0.0f;
        
        const float h2 = h * h;
        const float h4 = h2 * h2;
        const float h8 = h4 * h4;
        const float h9 = h8 * h;

        // Константа для 2D случая
        const float k = 4.0f / (kPi * h8);

        const float x = h2 - r * r;
        return k * x * x * x;
    }

    // Spiky gradient 
    inline float spikyGradCoeff(float r, float h = Config::smoothingRadius)
    {
        if (r < 0.0f || r >= h)
            return 0.0f;
        
        // Константа для 2D случая
        const float h5 = h * h * h * h * h;
        const float k = -5.0f / (kPi * h5);

        const float x = (h - r);

        // Возвращается только скалярный коэффциент.
        return k * x * x;
    }
}
