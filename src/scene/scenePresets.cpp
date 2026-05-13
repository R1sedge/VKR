#include "scene/ScenePresets.h"
#include "scene/SceneBuilder.h"
#include "common/Config.h"

namespace ScenePresets
{
    SceneDescription defaultFluid()
    {
        const float spacing = Config::particleRadius * 2.5f;

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
        const float spacing = Config::particleRadius * 2.5f;

        return SceneBuilder::create("Dam Break")
            .setBoxVessel(3.0f, 3.0f, 0.4f)
            .addFluidBox(-2.5f, -1.00f, 0.0f,
                          0.5f, 1.8f, 0.4f)
                .withSpacing(spacing)
                .withPhase(0)
                .withFilterByBoundary(true)
            .build();
    }

    SceneDescription twoPhase()
    {
        const float spacing = Config::particleRadius * 2.5f;

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
        const float spacing = Config::particleRadius * 2.5f;

        // Прямоугольная трапеция в плоскости XZ, CCW
        const std::vector<glm::vec2> tankPolygon =
        {
            { 2.0f,  -2.0f}, 
            { 2.0f,  2.0f}, 
            {-2.0f,  0.0f},
            {-2.0f, -2.0f} 

        };

        return SceneBuilder::create("Fuel Tank")
            .setConvexPrismVessel(tankPolygon, -1.f, 1.f)

            // Перегородка с отверстием
            .addInternalBaffleWithCircularHole(
                glm::vec3(-2/3.f, 0.f, -2/3.f),   // центр перегородки
                glm::vec3(1.f, 0.f, 0.f),   // нормаль (перегородка перпендикулярна X)
                glm::vec3(0.f, 1.f, 0.f),   // upHint
                4/3.0f,                     // halfWidth 
                1.f,                        // halfHeight
                0.00f,                      // thickness
                glm::vec2(0.f, 0.f),        // отверстие по центру
                0.35f)                      // радиус отверстия
            
            .addInternalBaffleWithCircularHole(
                glm::vec3(2/3.f, 0.f, -1/3.f),
                glm::vec3(1.f, 0.f, 0.f),
                glm::vec3(0.f, 1.f, 0.f),
                5/3.0f,
                1.f,
                0.00f,
                glm::vec2(0.f, 0.f),
                0.35f)


            // Жидкость
            .addFluidBox(0.0f, -0.55f, 0.0f,
                         1.75f, 0.55f, 1.00f)
                .withSpacing(spacing)
                .withPhase(0)
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

    int findByName(std::string name)
    {
        for (int i = 0; i < count(); i++)
        {
            if (name == s_names[i])
                return i;
        }
        return 0;
    }
}
