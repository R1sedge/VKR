#include "benchmarkScenes.h"

#include "sceneBuilder.h"
#include "common/Config.h"
#include <cmath>

namespace BenchmarkScenes 
{

    static constexpr float kSpacing   = Config::particleRadius * 2.05f;
    // жидкость занимает ~50% объёма сосуда
    static constexpr float kFillRatio = 0.50f;

    static float fluidSideForN(int N) 
    {
        const float r = Config::particleRadius;
        return (std::cbrt(static_cast<float>(N)) - 1.0f) * kSpacing + 2.0f * r;
    }

    SceneDescription makeBox(int targetN) 
    {
        const float r = Config::particleRadius;
        const float fluidSide = fluidSideForN(targetN);
        const float vesselHalf = (fluidSide / std::cbrt(kFillRatio)) * 0.5f;
        const float fluidHalf = fluidSide * 0.5f;

        const float fluidCY = -vesselHalf + r + fluidHalf;

        return SceneBuilder::create("BenchmarkBox")
            .setBoxVessel(vesselHalf, vesselHalf, vesselHalf)
            .addFluidBox(0.0f, fluidCY, 0.0f, fluidHalf, fluidHalf, fluidHalf)
            .withSpacing(kSpacing)
            .withPhase(0)
            .withFilterByBoundary(true)
            .build();
    }

    SceneDescription makeDamBreak(int targetN) {
        const float r = Config::particleRadius;
        const float fluidSide = fluidSideForN(targetN);
        const float vesselHalfX = fluidSide * 1.5f;
        const float vesselHalfYZ = (fluidSide / std::cbrt(kFillRatio)) * 0.5f;
        const float fluidHalf = fluidSide * 0.5f;

        // Блок у левой стенки
        const float fluidCX = -vesselHalfX + r + fluidHalf;
        const float fluidCY = -vesselHalfYZ + r + fluidHalf;

        return SceneBuilder::create("BenchmarkDamBreak")
            .setBoxVessel(vesselHalfX, vesselHalfYZ, vesselHalfYZ)
            .addFluidBox(fluidCX, fluidCY, 0.0f, fluidHalf, fluidHalf, fluidHalf)
            .withSpacing(kSpacing)
            .withPhase(0)
            .withFilterByBoundary(true)
            .build();
    }

}