#include "scene/ScenePresets.h"
#include "scene/SceneBuilder.h"
#include "common/Config.h"

namespace ScenePresets
{
    SceneDescription defaultFluid()
    {
        const float r = Config::particleRadius;
        const float step = r * 2.2f;

        // 3D-куб частиц: ~30×30×30
        const float halfW = 30.0f / 2 * step;
        const float halfH = 30.0f / 2 * step;
        const float halfD = 30.0f / 2 * step;

        return SceneBuilder::create("Default Fluid")
            .addRect(0.0f, 0.0f, 0.0f, halfW, halfH, halfD)
            .withPhase(0)
            .setGravity(Config::gravityX, Config::gravityY, Config::gravityZ)
            .setBounds(-4.267f, 4.267f, -2.4f, 2.4f, -1.5f, 1.5f)
            .build();
    }

    SceneDescription damBreak()
    {
        const float r = Config::particleRadius;
        const float step = r * 2.2f;

        // 3D Dam Break: высокий узкий куб слева + широкий низкий куб справа
        const float colHalfW = 10.0f * step;
        const float colHalfH = 30.0f * step;
        const float colHalfD = 10.0f * step;
        const float colCenterX = -50.0f * step;

        const float lakeHalfW = 35.0f * step;
        const float lakeHalfH = 6.0f * step;
        const float lakeHalfD = 10.0f * step;
        const float lakeCenterX = 20.0f * step;

        return SceneBuilder::create("Dam Break")
            .addRect(colCenterX, 0.0f, 0.0f, colHalfW, colHalfH, colHalfD)
            .withPhase(0)
            .addRect(lakeCenterX, -22.0f * step, 0.0f, lakeHalfW, lakeHalfH, lakeHalfD)
            .withPhase(0)
            .setGravity(Config::gravityX, Config::gravityY, Config::gravityZ)
            .setBounds(-4.267f, 4.267f, -2.4f, 2.4f, -1.5f, 1.5f)
            .build();
    }

    SceneDescription twoPhase()
    {
        const float r = Config::particleRadius;
        const float step = r * 2.2f;

        // 3D Rayleigh-Taylor: два куба друг над другом
        // Нижняя жидкость (фаза 0) — тяжёлая, верхняя (фаза 1) — лёгкая
        const float halfW = 40.0f / 2 * step;
        const float halfH = 16.0f / 2 * step;
        const float halfD = 16.0f / 2 * step;

        return SceneBuilder::create("Two Phase")
            .addRect(0.0f, -18.0f * step, 0.0f, halfW, halfH, halfD)
            .withPhase(0)
            .withMass(1.0f)
            .addRect(0.0f,  18.0f * step, 0.0f, halfW, halfH, halfD)
            .withPhase(1)
            .withMass(0.5f)
            .setGravity(Config::gravityX, Config::gravityY, Config::gravityZ)
            .setBounds(-4.267f, 4.267f, -2.4f, 2.4f, -1.5f, 1.5f)
            .build();
    }

    SceneDescription fuelTank()
    {
        const float r = Config::particleRadius;
        const float step = r * 2.2f;

        // 3D Fuel Tank: куб топлива + куб воздуха, оба с горизонтальным импульсом
        const float tankHalfW = 45.0f / 2 * step;
        const float fuelHalfH = 18.0f / 2 * step;
        const float fuelHalfD = 18.0f / 2 * step;
        const float airHalfH  = 8.0f / 2 * step;
        const float airHalfD  = 18.0f / 2 * step;
        const float airCenterY = fuelHalfH + airHalfH + step;

        return SceneBuilder::create("Fuel Tank")
            .addRect(0.0f, 0.0f, 0.0f, tankHalfW, fuelHalfH, fuelHalfD)
            .withPhase(0)
            .withMass(1.0f)
            .withVelocity(0.8f, 0.0f, 0.0f)
            .addRect(0.0f, airCenterY, 0.0f, tankHalfW, airHalfH, airHalfD)
            .withPhase(1)
            .withMass(0.2f)
            .withVelocity(0.8f, 0.0f, 0.0f)
            .setGravity(Config::gravityX, Config::gravityY, Config::gravityZ)
            .setBounds(-4.267f, 4.267f, -2.4f, 2.4f, -1.5f, 1.5f)
            .build();
    }

    // ImGui Combo
    static const char* s_names[] = 
    {
        "Default Fluid",
        "Dam Break",
        "Two Phase",
        "Fuel Tank",
    };

    const char* const* names() { return s_names; }
    int count() { return 4; }

    SceneDescription getByIndex(int index)
    {
        switch (index)
        {
            case 0:  return defaultFluid();
            case 1:  return damBreak();
            case 2:  return twoPhase();
            case 3:  return fuelTank();
            default: return defaultFluid();
        }
    }
}
