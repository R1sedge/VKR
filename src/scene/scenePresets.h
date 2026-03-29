#pragma once

#include "scene/SceneDescription.h"

namespace ScenePresets
{
    SceneDescription defaultFluid();
    SceneDescription damBreak();
    SceneDescription twoPhase();
    SceneDescription fuelTank();

    // Список имён для ImGui Combo (порядок совпадает с getByIndex).
    const char* const* names();
    int count();
    SceneDescription getByIndex(int index);
}