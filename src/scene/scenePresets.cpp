#include "scene/ScenePresets.h"
#include "scene/SceneBuilder.h"
#include "common/Config.h"

namespace ScenePresets
{
    SceneDescription defaultFluid()
    {
        const float r = Config::particleRadius;
        const float step = r * 2.2f;

        const float halfW = 125.0f / 2 * step;
        const float halfH = 80.0f / 2 * step;

        return SceneBuilder::create("Default Fluid")
            .addRect(0.0f, 0.0f, halfW, halfH)
            .withPhase(0)
            .setGravity(Config::gravityX, Config::gravityY)
            .build();
    }

    SceneDescription damBreak()
    {
        const float r = Config::particleRadius;
        const float step = r * 2.2f;

        // Левый столб — высокий и узкий, начальный импульс вправо.
        // Правое «озеро» — низкое и широкое, стоит на месте.
        return SceneBuilder::create("Dam Break")
            .addRect(-55.0f * step, 0.0f, 12.0f * step, 35.0f * step)
            .withPhase(0)
            .addRect(20.0f * step, -27.0f * step, 40.0f * step, 8.0f * step)
            .withPhase(0)
            .setGravity(Config::gravityX, Config::gravityY)
            .build();
    }

    SceneDescription twoPhase()
    {
        const float r = Config::particleRadius;
        const float step = r * 2.2f;

        // Нижняя жидкость (фаза 0) — плотная, тяжёлая.
        // Верхняя жидкость (фаза 1) — лёгкая, лежит сверху.
        // Тест неустойчивости Рэлея-Тейлора.
        return SceneBuilder::create("Two Phase")
            .addRect(0.0f, -20.0f * step, 50.0f * step, 18.0f * step)
            .withPhase(0)
            .withMass(1.0f)
            .addRect(0.0f,  20.0f * step, 50.0f * step, 18.0f * step)
            .withPhase(1)
            .withMass(0.5f)
            .setGravity(Config::gravityX, Config::gravityY)
            .build();
    }

    SceneDescription fuelTank()
    {
        const float r = Config::particleRadius;
        const float step = r * 2.2f;

        // Топливо (фаза 0) — заполняет нижние 60% бака.
        // Воздух (фаза 1) — верхние 30%.
        // Начальный горизонтальный импульс имитирует манёвр самолёта.
        const float tankHalfW = 55.0f * step;
        const float fuelHalfH = 22.0f * step;
        const float airHalfH  = 10.0f * step;
        const float airCenterY = fuelHalfH + airHalfH + step;

        return SceneBuilder::create("Fuel Tank")
            .addRect(0.0f, 0.0f, tankHalfW, fuelHalfH)
            .withPhase(0)
            .withMass(1.0f)
            .withVelocity(0.8f, 0.0f)
            .addRect(0.0f, airCenterY, tankHalfW, airHalfH)
            .withPhase(1)
            .withMass(0.2f)
            .withVelocity(0.8f, 0.0f)
            .setGravity(Config::gravityX, Config::gravityY)
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
