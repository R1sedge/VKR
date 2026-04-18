// scene/SceneDescription.h
#pragma once
#include <vector>
#include <string>
#include "scene/fluidRegion.h"
#include "scene/boundary/vesselBoundary.h"

struct SceneDescription 
{
    std::string name;

    // Сосуд (бокс по умолчанию)
    VesselBoundary vessel;

    // Регионы частиц
    std::vector<FluidRegion> regions;
};