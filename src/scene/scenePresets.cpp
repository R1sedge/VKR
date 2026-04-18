#include "scene/ScenePresets.h"
#include "scene/SceneBuilder.h"
#include "common/Config.h"

namespace ScenePresets
{
    SceneDescription defaultFluid()
    {
        const float spacing = Config::particleRadius * 2.1f;

        return SceneBuilder::create("Default Fluid")
            .setBoxVessel(1.1f, 1.2f, 1.6f)
            .addFluidBox(0.0f, 0.0f, 0.0f,
                    1.0f, 1.15f, 1.0f)
                .withSpacing(spacing)
                .withPhase(0)
                .withFilterByBoundary(true)
            .build();
    }

    SceneDescription damBreak()
    {
        const float spacing = Config::particleRadius * 2.1f;

        return SceneBuilder::create("Dam Break")
            .setBoxVessel(3.2f, 2.3f, 1.6f)
            .addFluidBox(-1.75f, -0.40f, 0.0f,
                    0.95f, 1.45f, 0.95f)
                .withSpacing(spacing)
                .withPhase(0)
                .withFilterByBoundary(true)
            .build();
    }

    SceneDescription twoPhase()
    {
        const float spacing = Config::particleRadius * 2.1f;

        return SceneBuilder::create("Two Phase")
            .setBoxVessel(3.0f, 2.2f, 1.6f)

            // Левая жидкость
            .addFluidBox(-1.05f, -0.55f, 0.0f,
                    0.95f, 1.05f, 0.95f)
                .withSpacing(spacing)
                .withPhase(0)
                .withFilterByBoundary(true)

            // Правая жидкость
            .addFluidBox( 1.05f, -0.55f, 0.0f,
                     0.95f, 1.05f, 0.95f)
                .withSpacing(spacing)
                .withPhase(1)
                .withFilterByBoundary(true)
            .build();
    }

    SceneDescription fuelTank()
    {
        const float spacing = Config::particleRadius * 2.1f;

        // Прямоугольная трапеция в плоскости XZ, CCW
        const std::vector<glm::vec2> tankPolygon =
        {
            { 2.0f,  -2.0f}, 
            { 2.0f,  2.0f}, 
            {-2.0f,  0.0f},
            {-2.0f, -2.0f} 

        };

        return SceneBuilder::create("Fuel Tank")
            .setConvexPrismVessel(tankPolygon, -1.45f, 1.45f)

            // Основной объём жидкости
            .addFluidBox(0.0f, -0.80f, 0.0f,
                    1.75f, 0.55f, 1.00f)
                .withSpacing(spacing)
                .withPhase(0)
                .withFilterByBoundary(true)

            // Небольшой пузырь второй фазы
            .addFluidSphere(0.55f, 0.10f, 0.0f, 0.42f)
                .withSpacing(spacing)
                .withPhase(1)
                .withFilterByBoundary(true)
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
